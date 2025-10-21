#include "CoreMinimal.h"
#include "AICoreLog.h"
#include "AICoreSnapshot.h"
#include "HAL/IConsoleManager.h"
#include "String/LexFromString.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "rules.h"
#include "rules_utbg.h"
#include "tt.h"

#include <vector>
#include <algorithm>
#include <limits>
#include <climits>

//////////////////////////////////////////////////////////////////////////
// TU Probe
//////////////////////////////////////////////////////////////////////////

static struct FAICoreSearchTUProbe {
    FAICoreSearchTUProbe() {
        UE_LOG(LogAICore, Log, TEXT("[AICore] Search TU loaded"));
    }
} GAICoreSearchTUProbe;

//////////////////////////////////////////////////////////////////////////
// Transposition Table (global)
//////////////////////////////////////////////////////////////////////////

static TTable GAICoreTT;
static int32  GAICoreTTSizeMB = 64;

//////////////////////////////////////////////////////////////////////////
// Defaults (difficulty)
//////////////////////////////////////////////////////////////////////////

static int32 GAICoreDefaultSoftMs = 300;
static int32 GAICoreDefaultHardMs = 350;
static int32 GAICoreDefaultDepth = 5;
static int32 GAICoreDefaultRootK = -1;
static int32 GAICoreDefaultNodeK = -1;

//////////////////////////////////////////////////////////////////////////
// CVars
//////////////////////////////////////////////////////////////////////////

// Eval weights
static TAutoConsoleVariable<int32> CVarAICore_W_HP(TEXT("AICore.W_HP"), 100, TEXT("Weight: unit HP"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_W_Pos(TEXT("AICore.W_Pos"), 3, TEXT("Weight: positional closeness"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_W_ThreatFor(TEXT("AICore.W_ThreatFor"), 25, TEXT("Weight: friendly threats"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_W_ThreatAgainst(TEXT("AICore.W_ThreatAgainst"), 35, TEXT("Weight: enemy threats against us"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_W_Cohesion(TEXT("AICore.W_Coh"), 2, TEXT("Weight: ally cohesion"), ECVF_Default);

// Ordering weights
static TAutoConsoleVariable<int32> CVarAICore_OrderPos(TEXT("AICore.OrderPos"), 8, TEXT("Ordering weight: positional closeness"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_OrderThreat(TEXT("AICore.OrderThreat"), 6, TEXT("Ordering weight: threat-relief delta"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_OrderAPPenalty(TEXT("AICore.OrderAPPenalty"), 0, TEXT("Ordering penalty per AP cost"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_OrderCost(TEXT("AICore.OrderCost"), 0,TEXT("Ordering penalty per AP cost (higher => prefer cheaper actions first)"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_OrderEndTurnBias(TEXT("AICore.OrderEndTurnBias"), 0,TEXT("Ordering bias for EndTurn (positive pulls EndTurn up)"), ECVF_Default);

// Options
static TAutoConsoleVariable<int32> CVarAICore_QStrict(TEXT("AICore.QStrict"), 1, TEXT("Quiescence strict: 1=lethal or threat-relief attacks only"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_Dedup(TEXT("AICore.Dedup"), 1, TEXT("Action-order invariance dedup when topology changes"), ECVF_Default);

// Logging
static TAutoConsoleVariable<int32>   CVarAICore_LogSearch(TEXT("AICore.LogSearch"), 1, TEXT("Write a JSONL per AICore.Search"), ECVF_Default);
static TAutoConsoleVariable<FString> CVarAICore_LogPath(TEXT("AICore.LogPath"), TEXT("AICore/search.jsonl"), TEXT("Relative path under Saved/"), ECVF_Default);

// Overlay
static TAutoConsoleVariable<int32> CVarAICore_Overlay(TEXT("AICore.Overlay"), 1, TEXT("On-screen overlay after search"), ECVF_Default);

// Root tie-breaking epsilon-noise
static TAutoConsoleVariable<int32> CVarAICore_Epsilon(TEXT("AICore.Epsilon"), 0, TEXT("Percent [0..100]: pick randomly within ROOT top tie group"), ECVF_Default);
static TAutoConsoleVariable<int32> CVarAICore_NoiseSeed(TEXT("AICore.NoiseSeed"), 12345, TEXT("Deterministic RNG seed for root tie-breaking"), ECVF_Default);

// --- UTBG 액션 문자열화 ---
static FString ActionToString_UTBG(const Action& a)
{
    switch (a.type) {
    case ActionType::Move:    return FString::Printf(TEXT("Move(%d->%d)"), a.actorId, a.tileIndex);
    case ActionType::Attack:  return FString::Printf(TEXT("Attack(%d->%d)"), a.actorId, a.targetId);
    case ActionType::EndTurn: return TEXT("EndTurn");
    case ActionType::Pass:    return FString::Printf(TEXT("Pass(%d)"), a.actorId);
    }
    return TEXT("Unknown");
}

//////////////////////////////////////////////////////////////////////////
//  UTBG: PV를 실제로 재적용하며 AP변화/EndTurn 여부를 JSONL로 남긴다
//////////////////////////////////////////////////////////////////////////

static void WriteUTBGSearchLogJSONL(
    const GameState& S0, const UTBGRules& R,
    const std::vector<Action>& PV,
    int bestScore, int maxDepth, int64 nodes, double ms)
{
    // 글로벌 스위치(AICore.LogSearch) 따라가기
    extern TAutoConsoleVariable<int32> CVarAICore_LogSearch;
    if (CVarAICore_LogSearch.GetValueOnAnyThread() == 0) return;

    GameState S = S0; // 안전하게 복사해 재생
    FString actionsJson; actionsJson.Reserve(512);

    actionsJson += TEXT("[");
    bool first = true;
    for (const Action& a : PV)
    {
        UTBGDelta dx{};
        R.make(S, a, dx); // 적용(팀AP/턴 처리 포함)

        const int sideBefore = (int)dx.SideBefore;
        const int ap0_before = (int)dx.APBefore[0];
        const int ap1_before = (int)dx.APBefore[1];
        const int ap0_after = S.teamAP[0];
        const int ap1_after = S.teamAP[1];
        const bool flipped = (dx.bFlippedTurn != 0);

        if (!first) actionsJson += TEXT(",");
        first = false;

        actionsJson += FString::Printf(
            TEXT("{\"act\":\"%s\",\"side\":%d,\"ap_before\":[%d,%d],\"ap_after\":[%d,%d],\"flipped\":%s}"),
            *ActionToString_UTBG(a), sideBefore,
            ap0_before, ap1_before,
            ap0_after, ap1_after,
            flipped ? TEXT("true") : TEXT("false")
        );
        // S는 누적 적용 상태로 유지(rollback 불필요)
    }
    actionsJson += TEXT("]");

    // 파일 경로
    const FString rel = TEXT("AICore/utbg_search.jsonl"); // 전용 파일
    const FString dir = FPaths::Combine(FPaths::ProjectSavedDir(), FPaths::GetPath(rel));
    const FString file = FPaths::Combine(FPaths::ProjectSavedDir(), rel);
    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    if (!PF.DirectoryExists(*dir)) PF.CreateDirectoryTree(*dir);

    const uint64 ts_ms = (uint64)(FDateTime::UtcNow().ToUnixTimestamp() * 1000LL);
    const FString line = FString::Printf(
        TEXT("{\"ts\":%llu,\"depth\":%d,\"nodes\":%lld,\"ms\":%.3f,\"score\":%d,")
        TEXT("\"teamAP_start\":[%d,%d],\"side_start\":%d,")
        TEXT("\"actions\":%s}\n"),
        (unsigned long long)ts_ms, maxDepth, (long long)nodes, ms, bestScore,
        S0.teamAP[0], S0.teamAP[1], S0.sideToAct,
        *actionsJson
    );

    FFileHelper::SaveStringToFile(line, *file, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}



//////////////////////////////////////////////////////////////////////////
// TT validity on eval-weight change
//////////////////////////////////////////////////////////////////////////

static struct FEvalWeightsSnapshot {
    int W_HP = 100, W_Pos = 3, W_TFor = 25, W_TAgainst = 35, W_Coh = 2;
} GLastEval;

static void EnsureTTValidityOnWeightsChange()
{
    const int nowHP     = CVarAICore_W_HP.GetValueOnAnyThread();
    const int nowPos    = CVarAICore_W_Pos.GetValueOnAnyThread();
    const int nowTF     = CVarAICore_W_ThreatFor.GetValueOnAnyThread();
    const int nowTA     = CVarAICore_W_ThreatAgainst.GetValueOnAnyThread();
    const int nowCoh    = CVarAICore_W_Cohesion.GetValueOnAnyThread();

    if (nowHP != GLastEval.W_HP || nowPos != GLastEval.W_Pos || nowTF != GLastEval.W_TFor || nowTA != GLastEval.W_TAgainst || nowCoh != GLastEval.W_Coh)
    {
        GAICoreTT.ResizeMB(128);
        GAICoreTT.ResizeMB(GAICoreTTSizeMB);
        UE_LOG(LogAICore, Log, TEXT("[TT] Cleared due to eval weight change"));
        GLastEval = { nowHP, nowPos, nowTF, nowTA, nowCoh };
    }
}

//////////////////////////////////////////////////////////////////////////
// Small RNG (deterministic)
//////////////////////////////////////////////////////////////////////////

static FORCEINLINE uint64 XS64(uint64& s) {
    s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
    return s * 2685821657736338717ULL;
}
static FORCEINLINE int RandRange(uint64& s, int n) {
    return (n > 0) ? int(XS64(s) % (uint64)n) : 0;
}

//////////////////////////////////////////////////////////////////////////
// Time manager
//////////////////////////////////////////////////////////////////////////

struct FTimeBudget { int32 SoftMs = 300; int32 HardMs = 350; };

class FTimeManager {
    double StartS = 0.0;
    FTimeBudget B;
public:
    void Start(const FTimeBudget& In) { B = In; StartS = FPlatformTime::Seconds(); }
    FORCEINLINE double ElapsedMs() const { return (FPlatformTime::Seconds() - StartS) * 1000.0; }
    FORCEINLINE bool SoftExpired() const { return ElapsedMs() >= B.SoftMs; }
    FORCEINLINE bool HardExpired() const { return ElapsedMs() >= B.HardMs; }
};

//////////////////////////////////////////////////////////////////////////
// Namespace
//////////////////////////////////////////////////////////////////////////

namespace AICore {

    constexpr int  INF = 1000000000;
    constexpr int  kAttackDamage = 5;

    //////////////////////////////////////////////////////////////////////////
    // Params & Stats
    //////////////////////////////////////////////////////////////////////////

    struct EvalWeights {
        int HP = 100, Pos = 3, TFor = 25, TAgainst = 35, Coh = 2;
    };
    struct OrderWeights {
        int Pos = 8;
        int Threat = 6;
        int Cost = 0;
        int EndTurnBias = 0;
    };
    struct SearchParams {
        FTimeBudget Budget{};
        int MaxDepth = 5;
        int RootK = -1;
        int NodeK = -1;
        bool Dedup = true;
        bool QStrict = true;
        int  Epsilon = 0;
        int  NoiseSeed = 12345;
        int  AttackDamage = kAttackDamage;
        EvalWeights  E{};
        OrderWeights O{};
    };

    struct SearchStats {
        int64 Nodes = 0;
        int64 TTHits = 0;
        int64 TTExact = 0;
        int64 TTLower = 0;
        int64 TTUpper = 0;
        int64 QCalls = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // Context
    //////////////////////////////////////////////////////////////////////////

    struct SearchCtx {
        BasicRules* Rules = nullptr;
        FTimeManager* TM = nullptr;
        TTable* TT = nullptr;
        uint16 Age = 0;
        SearchStats Stats{};
        const SearchParams* P = nullptr;
    };

    //////////////////////////////////////////////////////////////////////////
    // Utilities
    //////////////////////////////////////////////////////////////////////////

    static FORCEINLINE int Manhattan(int a, int b, int W) {
        const int ax = a % W, ay = a / W, bx = b % W, by = b / W;
        return FMath::Abs(ax - bx) + FMath::Abs(ay - by);
    }

    static FORCEINLINE uint64 KeyAfterFlip(const GameState& S) {
        return (S.key ^ S.Z.sideToAct[S.sideToAct] ^ S.Z.sideToAct[S.sideToAct ^ 1]);
    }

    static FORCEINLINE void FlipSide(GameState& S) {
        S.key ^= S.Z.sideToAct[S.sideToAct];
        S.sideToAct ^= 1;
        S.key ^= S.Z.sideToAct[S.sideToAct];
    }

    // Is this attack lethal under the current damage model?
    static FORCEINLINE bool IsLethalAttack(const GameState& S, const Action& a, int fallbackDamage) {
        if (a.type != ActionType::Attack || a.targetId < 0) return false;
        const int dmg = (a.actorId >= 0 && a.actorId < (int)S.units.size() && S.units[a.actorId].attack > 0)
            ? S.units[a.actorId].attack : fallbackDamage;
        return (S.units[a.targetId].hp - dmg) <= 0;
    }

    // RAII guard for make/unmake safety
    struct FScopedMake {
        BasicRules& R;
        GameState& S;
        Delta D{};
        bool Active{ false };

        FScopedMake(BasicRules& InR, GameState& InS, const Action& A) : R(InR), S(InS) {
            R.make(S, A, D);
            Active = true;
        }
        ~FScopedMake() {
            if (Active) R.unmake(S, D);
        }
        FScopedMake(const FScopedMake&) = delete;
        FScopedMake& operator=(const FScopedMake&) = delete;
        void Release() { Active = false; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Heuristics
    //////////////////////////////////////////////////////////////////////////

    static int NearestEnemyDistFrom(const GameState& S, int tile, int myTeam) {
        int best = INT_MAX;
        for (const auto& e : S.units) {
            if (!e.alive || e.team == myTeam || e.tile < 0) continue;
            best = FMath::Min(best, Manhattan(tile, e.tile, S.width));
        }
        return (best == INT_MAX) ? 0 : best;
    }

    static int CountAdjThreatPairs(const GameState& S, int teamA, int teamB) {
        int cnt = 0;
        for (const auto& ua : S.units) {
            if (!ua.alive || ua.tile < 0 || ua.team != teamA) continue;
            for (const auto& ub : S.units) {
                if (!ub.alive || ub.tile < 0 || ub.team != teamB) continue;
                if (Manhattan(ua.tile, ub.tile, S.width) == 1) ++cnt;
            }
        }
        return cnt;
    }

    static int NearestAllyDistFrom(const GameState& S, int tile, int myTeam) {
        int best = INT_MAX;
        for (const auto& a : S.units) {
            if (!a.alive || a.tile < 0 || a.team != myTeam) continue;
            const int d = Manhattan(tile, a.tile, S.width);
            if (d > 0) best = FMath::Min(best, d); // exclude self
        }
        return best;
    }

    static int SumAllyCohesion(const GameState& S, int team) {
        int sum = 0;
        for (const auto& u : S.units) {
            if (!u.alive || u.tile < 0 || u.team != team) continue;
            const int d = NearestAllyDistFrom(S, u.tile, team);
            sum += (d == 1) ? 2 : (d == 2) ? 1 : 0;
        }
        return sum;
    }

    static int Eval(const GameState& S, const EvalWeights& W)
    {
        const int me = S.sideToAct;
        const int them = me ^ 1;
        int score = 0;

        // 1) HP / Position
        for (const auto& u : S.units) {
            if (!u.alive || u.tile < 0) continue;
            const int sign = (u.team == me) ? +1 : -1;
            score += sign * (W.HP * u.hp);

            const int d = NearestEnemyDistFrom(S, u.tile, u.team);
            const int proximity = (d > 0) ? (10 - FMath::Min(d, 10)) : 0;
            score += sign * (W.Pos * proximity);
        }

        // 2) Threats (adjacent pairs)
        const int threatsFor = CountAdjThreatPairs(S, me, them);
        const int threatsAgainst = CountAdjThreatPairs(S, them, me);
        score += W.TFor * threatsFor - W.TAgainst * threatsAgainst;

        // 3) Ally cohesion
        const int cohMe = SumAllyCohesion(S, me);
        const int cohThem = SumAllyCohesion(S, them);
        score += W.Coh * (cohMe - cohThem);

        return score;
    }

    static int AdjacentEnemyCountAtTile(const GameState& S, int tile, int myTeam) {
        if (tile < 0) return 0;
        const int W = S.width, H = S.height;
        const int x = tile % W, y = tile / W;

        auto ToIdx = [&](int tx, int ty)->int {
            if (tx < 0 || ty < 0 || tx >= W || ty >= H) return -1;
            return ty * W + tx;
            };

        const int nb[4] = { ToIdx(x - 1,y), ToIdx(x + 1,y), ToIdx(x,y - 1), ToIdx(x,y + 1) };
        int cnt = 0;
        for (int k = 0; k < 4; ++k) {
            const int idx = nb[k];
            if (idx < 0) continue;
            for (const auto& u : S.units) {
                if (u.alive && u.tile == idx && u.team != myTeam) { ++cnt; break; }
            }
        }
        return cnt;
    }

    static int ThreatReliefForMove(const GameState& S, const Action& a) {
        if (a.type != ActionType::Move) return 0;
        const auto& u = S.units[a.actorId];
        const int before = AdjacentEnemyCountAtTile(S, u.tile, u.team);
        const int after = AdjacentEnemyCountAtTile(S, a.tileIndex, u.team);
        return (before - after);
    }

    static int ThreatReliefForAttack(const GameState& S, const Action& a, int attackDamage) {
        if (a.type != ActionType::Attack || a.targetId < 0) return 0;
        const auto& u = S.units[a.actorId];
        const auto& t = S.units[a.targetId];
        if (u.tile < 0 || t.tile < 0) return 0;

        const bool adjacent = Manhattan(u.tile, t.tile, S.width) == 1;
        const bool lethal = IsLethalAttack(S, a, attackDamage);
        return (adjacent && lethal) ? 1 : 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // Move ordering
    //////////////////////////////////////////////////////////////////////////

    static int ScoreActionForOrdering(const GameState& S, const Action& a, const OrderWeights& OW, int fallbackDamage)
    {
        int sc = 0;

        if (a.type == ActionType::Attack && a.targetId >= 0) 
        {
            const auto& t = S.units[a.targetId];
            const int dmg = FMath::Clamp(
                (a.actorId >= 0 && a.actorId < (int)S.units.size() && S.units[a.actorId].attack > 0)
                ? S.units[a.actorId].attack : fallbackDamage, 0, t.hp);
            const int remaining = t.hp - dmg;

            if (t.hp - dmg <= 0) sc += 5000;           // 확정 킬 최우선
            sc += dmg * 10;                            // 가한 피해량 보너스
            sc += ThreatReliefForAttack(S, a, dmg > 0 ? dmg : fallbackDamage) * OW.Threat;
        }
        else if (a.type == ActionType::Move && a.tileIndex >= 0) 
        {
            const auto& u = S.units[a.actorId];
            const int before = NearestEnemyDistFrom(S, u.tile, u.team);
            const int after = NearestEnemyDistFrom(S, a.tileIndex, u.team);
            sc += (before - after) * OW.Pos;          // get closer to enemy
            sc += ThreatReliefForMove(S, a) * OW.Threat;
        }
        else if (a.type == ActionType::Pass) 
        {
            sc -= 5;
        }
        else if (a.type == ActionType::EndTurn) 
        {
            sc += OW.EndTurnBias;
        }

        // Deterministic tie-breaker
        sc = (sc << 1) | int(a.signature() & 1ULL);
        sc -= CVarAICore_OrderAPPenalty.GetValueOnAnyThread() * a.apCost;
        sc -= OW.Cost * a.apCost;
        return sc;
    }

    static void SortActionsDeterministic(const GameState& S, std::vector<Action>& moves, const OrderWeights& OW, int attackDamage)
    {
        std::stable_sort(moves.begin(), moves.end(), [&](const Action& A, const Action& B) {
            const int sa = ScoreActionForOrdering(S, A, OW, attackDamage);
            const int sb = ScoreActionForOrdering(S, B, OW, attackDamage);
            if (sa != sb) return sa > sb;
            return A.signature() < B.signature();
            });
    }

    static void PreferBestMove(std::vector<Action>& mv, const Action& best)
    {
        if (best.actorId < 0) return;
        auto it = std::find_if(mv.begin(), mv.end(), [&](const Action& x) {
            return x.signature() == best.signature();
            });
        if (it != mv.end()) std::iter_swap(mv.begin(), it);
    }

    //////////////////////////////////////////////////////////////////////////
    // Logging (JSONL)
    //////////////////////////////////////////////////////////////////////////

    static void WriteSearchLogJSONL(
        int depth, int64 nodes, double ms, int bestScore, const std::vector<Action>& pv, const EvalWeights& W)
    {
        if (CVarAICore_LogSearch.GetValueOnAnyThread() == 0) return;

        const FString rel   = CVarAICore_LogPath.GetValueOnAnyThread();
        const FString dir   = FPaths::Combine(FPaths::ProjectSavedDir(), FPaths::GetPath(rel));
        const FString file  = FPaths::Combine(FPaths::ProjectSavedDir(), rel);
        IPlatformFile& PF   = FPlatformFileManager::Get().GetPlatformFile();
        if (!PF.DirectoryExists(*dir)) PF.CreateDirectoryTree(*dir);

        // Serialize PV
        FString pvText; pvText.Reserve(256);
        for (size_t i = 0; i < pv.size(); ++i) {
            const Action& a = pv[i];
            if (a.type == ActionType::Move)   pvText += FString::Printf(TEXT("Move(%d->%d)"), a.actorId, a.tileIndex);
            else if (a.type == ActionType::Attack) pvText += FString::Printf(TEXT("Attack(%d->%d)"), a.actorId, a.targetId);
            else                                 pvText += FString::Printf(TEXT("Pass(%d)"), a.actorId);
            if (i + 1 < pv.size()) pvText += TEXT(" -> ");
        }

        const uint64 ts_ms = (uint64)(FDateTime::UtcNow().ToUnixTimestamp() * 1000LL);
        const FString line = FString::Printf(
            TEXT("{\"ts\":%llu,\"depth\":%d,\"nodes\":%lld,\"ms\":%.3f,\"score\":%d,")
            TEXT("\"W_HP\":%d,\"W_Pos\":%d,\"W_TFor\":%d,\"W_TAgainst\":%d,\"W_Coh\":%d,")
            TEXT("\"pv\":\"%s\"}\n"),
            (unsigned long long)ts_ms, depth, (long long)nodes, ms, bestScore,
            W.HP, W.Pos, W.TFor, W.TAgainst, W.Coh, *pvText);

        FFileHelper::SaveStringToFile(line, *file, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
    }

    //////////////////////////////////////////////////////////////////////////
    // Quiescence
    //////////////////////////////////////////////////////////////////////////

    static int Quiescence(GameState& S, int alpha, int beta, SearchCtx& Ctx)
    {
        Ctx.Stats.QCalls++;
        if (Ctx.TM->HardExpired()) return Eval(S, Ctx.P->E);

        int stand = Eval(S, Ctx.P->E);
        if (stand >= beta) return beta;
        if (stand > alpha) alpha = stand;

        std::vector<Action> mv;
        Ctx.Rules->generateLegal(S, mv);

        // keep only attacks
        mv.erase(std::remove_if(mv.begin(), mv.end(), [](const Action& a) {
            return a.type != ActionType::Attack;
            }), mv.end());

        if (Ctx.P->QStrict) {
            mv.erase(std::remove_if(mv.begin(), mv.end(), [&](const Action& a) {
                if (IsLethalAttack(S, a, Ctx.P->AttackDamage)) return false;
                return ThreatReliefForAttack(S, a, Ctx.P->AttackDamage) <= 0;
                }), mv.end());
        }

        if (mv.empty()) return alpha;

        SortActionsDeterministic(S, mv, Ctx.P->O, Ctx.P->AttackDamage);

        for (const auto& a : mv) {
            if (S.units[a.actorId].ap < a.apCost) continue;
            FScopedMake guard(*Ctx.Rules, S, a);
            FlipSide(S);
            Ctx.Stats.Nodes++;

            const int sc = -Quiescence(S, -beta, -alpha, Ctx);

            FlipSide(S);
            if (sc >= beta) return beta;
            if (sc > alpha) alpha = sc;

            if (Ctx.TM->HardExpired()) break;
        }
        return alpha;
    }

    //////////////////////////////////////////////////////////////////////////
    // AlphaBeta with TT + PV + dedup
    //////////////////////////////////////////////////////////////////////////

    static int AlphaBeta(GameState& S, int depth, int alpha, int beta, SearchCtx& Ctx, std::vector<Action>& outPV)
    {
        if (Ctx.TM->HardExpired()) return Eval(S, Ctx.P->E);

        const int alphaOrig = alpha;

        // TT probe
        TTEntry ent;
        const bool haveTT = (Ctx.TT && Ctx.TT->Probe(S.key, ent));
        if (haveTT && ent.Depth >= depth) {
            Ctx.Stats.TTHits++;
            if (ent.Bound == ETTBound::Exact) {
                Ctx.Stats.TTExact++;
                outPV.clear(); outPV.push_back(ent.BestMove);
                return ent.Score;
            }
            if (ent.Bound == ETTBound::Lower && ent.Score >= beta) { Ctx.Stats.TTLower++; return ent.Score; }
            if (ent.Bound == ETTBound::Upper && ent.Score <= alpha) { Ctx.Stats.TTUpper++; return ent.Score; }
        }

        if (depth == 0) return Quiescence(S, alpha, beta, Ctx);

        std::vector<Action> mv;
        Ctx.Rules->generateLegal(S, mv);

        // prune illegal by AP
        mv.erase(std::remove_if(mv.begin(), mv.end(), [&](const Action& a) {
            return S.units[a.actorId].ap < a.apCost;
            }), mv.end());

        if (mv.empty()) return Eval(S, Ctx.P->E);

        SortActionsDeterministic(S, mv, Ctx.P->O, Ctx.P->AttackDamage);
        if (haveTT && ent.BestMove.actorId != -1) PreferBestMove(mv, ent.BestMove);

        if (Ctx.P->NodeK > 0 && (int)mv.size() > Ctx.P->NodeK)
            mv.resize(Ctx.P->NodeK);

        const bool bDedup = Ctx.P->Dedup;
        std::vector<uint64> seenChildKeys; if (bDedup) seenChildKeys.reserve(mv.size());

        int best = std::numeric_limits<int>::min();
        std::vector<Action> bestPV;

        for (const auto& a : mv) {
            FScopedMake guard(*Ctx.Rules, S, a);

            bool skip = false;
            if (bDedup && (a.type == ActionType::Move || IsLethalAttack(S, a, Ctx.P->AttackDamage))) {
                const uint64 childKeyPostFlip = KeyAfterFlip(S);
                if (std::find(seenChildKeys.begin(), seenChildKeys.end(), childKeyPostFlip) != seenChildKeys.end())
                    skip = true;
                else
                    seenChildKeys.push_back(childKeyPostFlip);
            }

            if (!skip) {
                FlipSide(S);
                Ctx.Stats.Nodes++;

                std::vector<Action> childPV;
                const int sc = -AlphaBeta(S, depth - 1, -beta, -alpha, Ctx, childPV);

                FlipSide(S);

                if (sc > best) {
                    best = sc;
                    bestPV.clear(); bestPV.push_back(a);
                    bestPV.insert(bestPV.end(), childPV.begin(), childPV.end());
                }
                if (best > alpha) alpha = best;
                if (alpha >= beta || Ctx.TM->HardExpired()) {
                    // guard destructor will unmake
                    break;
                }
            }
            // guard destructor will unmake
        }

        // TT store
        if (Ctx.TT) {
            ETTBound b = ETTBound::Exact;
            if (best <= alphaOrig)      b = ETTBound::Upper;
            else if (best >= beta)      b = ETTBound::Lower;

            const Action storeBest = bestPV.empty() ? Action{} : bestPV.front();
            Ctx.TT->Store(S.key, (int16)depth, best, b, storeBest, Ctx.Age);
        }

        outPV = std::move(bestPV);
        return best;
    }

    //////////////////////////////////////////////////////////////////////////
    // Root (IDDFS + PW + dedup)
    //////////////////////////////////////////////////////////////////////////

    static void SearchRoot_IDDFS(
        GameState& S, BasicRules& R,
        const SearchParams& P,
        std::vector<Action>& OutPV, int& OutScore, int64& OutNodes, double& OutMs)
    {
        if (!GAICoreTT.IsReady()) {
            GAICoreTT.ResizeMB(64);
            UE_LOG(LogAICore, Log, TEXT("[TT] Initialized 64MB"));
        }

        FTimeManager TM; TM.Start(P.Budget);
        SearchCtx Ctx{ &R, &TM, &GAICoreTT, 0, {}, &P };

        std::vector<Action> rootMoves;
        R.generateLegal(S, rootMoves);
        rootMoves.erase(std::remove_if(rootMoves.begin(), rootMoves.end(), [&](const Action& a) {
            return S.units[a.actorId].ap < a.apCost;
            }), rootMoves.end());

        SortActionsDeterministic(S, rootMoves, P.O, P.AttackDamage);

        if (P.RootK > 0 && (int)rootMoves.size() > P.RootK)
            rootMoves.resize(P.RootK);

        // epsilon noise at root (top tie group)
        const int Eps = P.Epsilon;
        if (Eps > 0 && !rootMoves.empty()) {
            const int topScore = ScoreActionForOrdering(S, rootMoves[0], P.O, P.AttackDamage);
            int tieCount = 1;
            while (tieCount < (int)rootMoves.size() &&
                ScoreActionForOrdering(S, rootMoves[tieCount], P.O, P.AttackDamage) == topScore)
            {
                ++tieCount;
            }
            if (tieCount > 1) {
                uint64 seed = (uint64)P.NoiseSeed;
                seed ^= (uint64)S.key;
                seed ^= (uint64)P.MaxDepth * 0x9E3779B97F4A7C15ULL;

                const int roll = (int)(XS64(seed) % 100ULL);
                if (roll < Eps) {
                    const int pick = RandRange(seed, tieCount); // [0, tieCount)
                    if (pick > 0) std::iter_swap(rootMoves.begin(), rootMoves.begin() + pick);
                }
            }
        }

        int bestScore = std::numeric_limits<int>::min();
        std::vector<Action> bestPV;

        for (int depth = 1; depth <= P.MaxDepth; ++depth) {
            if (TM.SoftExpired()) break;
            Ctx.Age = (uint16)depth;

            int iterBest = std::numeric_limits<int>::min();
            std::vector<Action> iterPV;

            const bool bDedup = P.Dedup;
            std::vector<uint64> seenChildKeysRoot; if (bDedup) seenChildKeysRoot.reserve(rootMoves.size());

            for (const auto& a : rootMoves) {
                if (TM.SoftExpired()) break;

                FScopedMake guard(R, S, a);

                bool skip = false;
                if (bDedup && (a.type == ActionType::Move || IsLethalAttack(S, a, P.AttackDamage))) {
                    const uint64 childKeyPostFlip = KeyAfterFlip(S);
                    if (std::find(seenChildKeysRoot.begin(), seenChildKeysRoot.end(), childKeyPostFlip) != seenChildKeysRoot.end())
                        skip = true;
                    else
                        seenChildKeysRoot.push_back(childKeyPostFlip);
                }

                if (!skip) {
                    FlipSide(S);
                    std::vector<Action> childPV;
                    const int sc = -AlphaBeta(S, depth - 1, -INF, +INF, Ctx, childPV);
                    FlipSide(S);

                    if (sc > iterBest ||
                        (sc == iterBest && a.signature() < (iterPV.empty() ? ~0ULL : iterPV.front().signature())))
                    {
                        iterBest = sc;
                        iterPV.clear(); iterPV.push_back(a);
                        iterPV.insert(iterPV.end(), childPV.begin(), childPV.end());
                    }
                }
                // unmake by guard dtor
            }

            if (!iterPV.empty()) { bestScore = iterBest; bestPV = iterPV; }

            // reorder root with last best
            if (!bestPV.empty()) {
                const uint64 sig = bestPV.front().signature();
                std::stable_sort(rootMoves.begin(), rootMoves.end(), [&](const Action& A, const Action& B) {
                    if (A.signature() == sig) return true;
                    if (B.signature() == sig) return false;
                    const int sa = ScoreActionForOrdering(S, A, P.O, P.AttackDamage);
                    const int sb = ScoreActionForOrdering(S, B, P.O, P.AttackDamage);
                    if (sa != sb) return sa > sb;
                    return A.signature() < B.signature();
                    });
            }
        }

        OutPV = bestPV;
        OutScore = bestScore;
        OutNodes = Ctx.Stats.Nodes;
        OutMs = TM.ElapsedMs();

        WriteSearchLogJSONL(P.MaxDepth, OutNodes, OutMs, OutScore, OutPV, P.E);
    }

} // namespace AICore


//////////////////////////////////////////////////////////////////////////
// UTBG 전용: FlipSide 를 make/unmake가 관리 -> 탐색에서 호출 금지
//////////////////////////////////////////////////////////////////////////

namespace {

    // 템플릿 RAII (Delta 타입을 외부에서 넘길 수 있게)
    template<typename TRules, typename TDelta>
    struct FScopedMakeT {
        TRules& R;
        GameState& S;
        TDelta     D{};
        bool       Active{ false };

        FScopedMakeT(TRules& InR, GameState& InS, const Action& A) : R(InR), S(InS) {
            R.make(S, A, D); Active = true;
        }
        ~FScopedMakeT() { if (Active) R.unmake(S, D); }
        void Release() { Active = false; }
    };

    struct SearchCtxUTBG {
        FTimeManager* TM = nullptr;
        TTable* TT = nullptr;   // 필요하면 nullptr로 비워 TT 미사용
        int64         Nodes = 0;
        int64         TTHits = 0;
    };

    static int Quiescence_UTBG(GameState& S, int alpha, int beta,
        UTBGRules& R, SearchCtxUTBG& Ctx)
    {
        if (Ctx.TM->HardExpired()) return AICore::Eval(S, {
            CVarAICore_W_HP.GetValueOnAnyThread(),
            CVarAICore_W_Pos.GetValueOnAnyThread(),
            CVarAICore_W_ThreatFor.GetValueOnAnyThread(),
            CVarAICore_W_ThreatAgainst.GetValueOnAnyThread(),
            CVarAICore_W_Cohesion.GetValueOnAnyThread()
            });

        int stand = AICore::Eval(S, {
            CVarAICore_W_HP.GetValueOnAnyThread(),
            CVarAICore_W_Pos.GetValueOnAnyThread(),
            CVarAICore_W_ThreatFor.GetValueOnAnyThread(),
            CVarAICore_W_ThreatAgainst.GetValueOnAnyThread(),
            CVarAICore_W_Cohesion.GetValueOnAnyThread()
            });

        if (stand >= beta) return beta;
        if (stand > alpha) alpha = stand;

        std::vector<Action> mv;
        R.generateLegal(S, mv);
        mv.erase(std::remove_if(mv.begin(), mv.end(),
            [](const Action& a) { return a.type != ActionType::Attack; }), mv.end());

        if (mv.empty()) return alpha;

        AICore::SortActionsDeterministic(
            S, mv,
            AICore::OrderWeights{
                CVarAICore_OrderPos.GetValueOnAnyThread(),
                CVarAICore_OrderThreat.GetValueOnAnyThread(),
                CVarAICore_OrderCost.GetValueOnAnyThread(),
                CVarAICore_OrderEndTurnBias.GetValueOnAnyThread()
            },
            /*attackDamage*/5);

        for (const auto& a : mv)
        {
            FScopedMakeT<UTBGRules, UTBGDelta> guard(R, S, a);
            ++Ctx.Nodes;

            const int sc = -Quiescence_UTBG(S, -beta, -alpha, R, Ctx);

            if (sc >= beta) return beta;
            if (sc > alpha) alpha = sc;

            if (Ctx.TM->HardExpired()) break;
        }
        return alpha;
    }

    static int AlphaBeta_UTBG(GameState& S, int depth, int alpha, int beta,
        UTBGRules& R, SearchCtxUTBG& Ctx, std::vector<Action>& outPV)
    {
        if (Ctx.TM->HardExpired()) return AICore::Eval(S, {
            CVarAICore_W_HP.GetValueOnAnyThread(),
            CVarAICore_W_Pos.GetValueOnAnyThread(),
            CVarAICore_W_ThreatFor.GetValueOnAnyThread(),
            CVarAICore_W_ThreatAgainst.GetValueOnAnyThread(),
            CVarAICore_W_Cohesion.GetValueOnAnyThread()
            });

        const int alphaOrig = alpha;

        // (옵션) TT probe - 현재는 teamAP가 해시에 없어 비활성 권장
        if (Ctx.TT)
        {
            TTEntry ent;
            if (Ctx.TT->Probe(S.key, ent) && ent.Depth >= depth)
            {
                Ctx.TTHits++;
                if (ent.Bound == ETTBound::Exact) { outPV.clear(); outPV.push_back(ent.BestMove); return ent.Score; }
                if (ent.Bound == ETTBound::Lower && ent.Score >= beta)  return ent.Score;
                if (ent.Bound == ETTBound::Upper && ent.Score <= alpha) return ent.Score;
            }
        }

        if (depth == 0)
            return Quiescence_UTBG(S, alpha, beta, R, Ctx);

        std::vector<Action> mv;
        R.generateLegal(S, mv);
        if (mv.empty())
            return AICore::Eval(S, {
                CVarAICore_W_HP.GetValueOnAnyThread(),
                CVarAICore_W_Pos.GetValueOnAnyThread(),
                CVarAICore_W_ThreatFor.GetValueOnAnyThread(),
                CVarAICore_W_ThreatAgainst.GetValueOnAnyThread(),
                CVarAICore_W_Cohesion.GetValueOnAnyThread()
                });

        AICore::SortActionsDeterministic(
            S, mv,
            AICore::OrderWeights{
                CVarAICore_OrderPos.GetValueOnAnyThread(),
                CVarAICore_OrderThreat.GetValueOnAnyThread()
            },
            /*attackDamage*/5);

        const int nodeK = GAICoreDefaultNodeK;
        if (nodeK > 0 && (int)mv.size() > nodeK) mv.resize(nodeK);

        const bool bDedup = (CVarAICore_Dedup.GetValueOnAnyThread() != 0);
        std::vector<uint64> seenKeys; if (bDedup) seenKeys.reserve(mv.size());

        int best = std::numeric_limits<int>::min();
        std::vector<Action> bestPV;

        for (const auto& a : mv)
        {
            FScopedMakeT<UTBGRules, UTBGDelta> guard(R, S, a);

            if (bDedup)
            {
                const uint64 k = S.key;
                if (std::find(seenKeys.begin(), seenKeys.end(), k) != seenKeys.end())
                    continue;
                seenKeys.push_back(k);
            }

            ++Ctx.Nodes;

            std::vector<Action> childPV;
            const int sc = -AlphaBeta_UTBG(S, depth - 1, -beta, -alpha, R, Ctx, childPV);

            if (sc > best) {
                best = sc;
                bestPV.clear(); bestPV.push_back(a);
                bestPV.insert(bestPV.end(), childPV.begin(), childPV.end());
            }
            if (best > alpha) alpha = best;
            if (alpha >= beta || Ctx.TM->HardExpired()) break;
        }

        if (Ctx.TT)
        {
            ETTBound b = ETTBound::Exact;
            if (best <= alphaOrig) b = ETTBound::Upper;
            else if (best >= beta) b = ETTBound::Lower;
            const Action storeBest = bestPV.empty() ? Action{} : bestPV.front();
            Ctx.TT->Store(S.key, (int16)depth, best, b, storeBest, /*age*/(uint16)depth);
        }

        outPV = std::move(bestPV);
        return best;
    }
}

//////////////////////////////////////////////////////////////////////////
// Console Commands (public API remains the same)
//////////////////////////////////////////////////////////////////////////

using namespace AICore;

// AICore.Difficulty <easy|normal|hard>
static void RunAICoreDifficulty(const TArray<FString>& Args, UWorld*)
{
    if (Args.Num() < 1) {
        UE_LOG(LogAICore, Log, TEXT("Usage: AICore.Difficulty <easy|normal|hard>"));
        return;
    }
    const FString Mode = Args[0].ToLower();

    if (Mode == TEXT("easy")) {
        GAICoreDefaultSoftMs = 150; GAICoreDefaultHardMs = 180; GAICoreDefaultDepth = 4;
        GAICoreDefaultRootK = 8;   GAICoreDefaultNodeK = 6;
        UE_LOG(LogAICore, Log, TEXT("[Difficulty] easy"));
    }
    else if (Mode == TEXT("normal")) {
        GAICoreDefaultSoftMs = 300; GAICoreDefaultHardMs = 350; GAICoreDefaultDepth = 5;
        GAICoreDefaultRootK = -1;  GAICoreDefaultNodeK = -1;
        UE_LOG(LogAICore, Log, TEXT("[Difficulty] normal"));
    }
    else if (Mode == TEXT("hard")) {
        GAICoreDefaultSoftMs = 500; GAICoreDefaultHardMs = 600; GAICoreDefaultDepth = 7;
        GAICoreDefaultRootK = 16;  GAICoreDefaultNodeK = 12;
        UE_LOG(LogAICore, Log, TEXT("[Difficulty] hard"));
    }
    else {
        UE_LOG(LogAICore, Warning, TEXT("[Difficulty] Unknown mode: %s"), *Mode);
    }
}
static FAutoConsoleCommandWithWorldAndArgs CmdAICoreDifficulty(
    TEXT("AICore.Difficulty"),
    TEXT("Usage: AICore.Difficulty <easy|normal|hard> // sets default search params"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICoreDifficulty)
);

// AICore.Search [softMs] [hardMs] [maxDepth] [rootK] [nodeK]
static void RunAICoreSearch(const TArray<FString>& Args, UWorld* /*World*/)
{
    EnsureTTValidityOnWeightsChange();

    // Defaults
    int32 SoftMs = GAICoreDefaultSoftMs;
    int32 HardMs = GAICoreDefaultHardMs;
    int32 MaxDepth = GAICoreDefaultDepth;
    int32 RootK = GAICoreDefaultRootK;
    int32 NodeK = GAICoreDefaultNodeK;

    if (Args.Num() >= 1) LexFromString(SoftMs, *Args[0]);
    if (Args.Num() >= 2) LexFromString(HardMs, *Args[1]);
    if (Args.Num() >= 3) LexFromString(MaxDepth, *Args[2]);
    if (Args.Num() >= 4) LexFromString(RootK, *Args[3]);
    if (Args.Num() >= 5) LexFromString(NodeK, *Args[4]);

    SearchParams P{};
    P.Budget.SoftMs = SoftMs;
    P.Budget.HardMs = HardMs;
    P.MaxDepth = MaxDepth;
    P.RootK = RootK;
    P.NodeK = NodeK;

    // Snapshot CVars once
    P.E.HP = CVarAICore_W_HP.GetValueOnAnyThread();
    P.E.Pos = CVarAICore_W_Pos.GetValueOnAnyThread();
    P.E.TFor = CVarAICore_W_ThreatFor.GetValueOnAnyThread();
    P.E.TAgainst = CVarAICore_W_ThreatAgainst.GetValueOnAnyThread();
    P.E.Coh = CVarAICore_W_Cohesion.GetValueOnAnyThread();

    P.O.Pos = CVarAICore_OrderPos.GetValueOnAnyThread();
    P.O.Threat = CVarAICore_OrderThreat.GetValueOnAnyThread();
    P.O.Cost = CVarAICore_OrderCost.GetValueOnAnyThread();
    P.O.EndTurnBias = CVarAICore_OrderEndTurnBias.GetValueOnAnyThread();

    P.QStrict = (CVarAICore_QStrict.GetValueOnAnyThread() != 0);
    P.Dedup = (CVarAICore_Dedup.GetValueOnAnyThread() != 0);
    P.Epsilon = CVarAICore_Epsilon.GetValueOnAnyThread();
    P.NoiseSeed = CVarAICore_NoiseSeed.GetValueOnAnyThread();

    // Example test state (same as original)
    GameState S;
    S.width = 5; S.height = 5; S.sideToAct = 0;
    S.units = { Unit{0,0,12,10,2,true}, Unit{1,1,13,10,2,true} };
    S.initZobrist(0xC0FFEEULL, (int)S.units.size());

    BasicRules R;

    std::vector<Action> PV;
    int score = 0;
    int64 nodes = 0;
    double ms = 0.0;

    SearchRoot_IDDFS(S, R, P, PV, score, nodes, ms);

    // Build PV text
    FString pvText;
    for (size_t i = 0; i < PV.size(); ++i) {
        const Action& a = PV[i];
        if (a.type == ActionType::Move)   pvText += FString::Printf(TEXT("Move(u=%d->%d)"), a.actorId, a.tileIndex);
        else if (a.type == ActionType::Attack) pvText += FString::Printf(TEXT("Attack(u=%d->t=%d)"), a.actorId, a.targetId);
        else                                pvText += FString::Printf(TEXT("Pass(u=%d)"), a.actorId);
        if (i + 1 < PV.size()) pvText += TEXT(" -> ");
    }

    const double nps = (ms > 0.0) ? (double)nodes / (ms / 1000.0) : 0.0;
    UE_LOG(LogAICore, Log, TEXT("[Search] bestScore=%d depth<=%d nodes=%lld time=%.2fms nps=%.0f"),
        score, MaxDepth, (long long)nodes, ms, nps);
    UE_LOG(LogAICore, Log, TEXT("[Search] PV: %s"), *pvText);

    if (CVarAICore_Overlay.GetValueOnAnyThread() != 0 && GEngine) {
        const FString line1 = FString::Printf(TEXT("[AICore] depth<=%d score=%d nodes=%lld time=%.2fms nps=%.0f"),
            MaxDepth, score, (long long)nodes, ms, nps);
        GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, line1);
        GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Silver, FString::Printf(TEXT("[PV] %s"), *pvText));
    }
}

static FAutoConsoleCommandWithWorldAndArgs CmdAICoreSearch(
    TEXT("AICore.Search"),
    TEXT("Usage: AICore.Search [softMs] [hardMs] [maxDepth] [rootK] [nodeK] omitted args use AICore.Difficulty defaults"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICoreSearch)
);

// AICore.TTResize <MB>
static void RunAICoreTTResize(const TArray<FString>& Args, UWorld*)
{
    int32 MB = 64;
    if (Args.Num() >= 1) LexFromString(MB, *Args[0]);
    GAICoreTTSizeMB = MB;
    GAICoreTT.ResizeMB(MB);
    UE_LOG(LogAICore, Log, TEXT("[TT] Resized to %d MB"), MB);
}
static FAutoConsoleCommandWithWorldAndArgs CmdAICoreTTResize(
    TEXT("AICore.TTResize"),
    TEXT("Usage: AICore.TTResize <MB>"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICoreTTResize)
);

// AICore.EvalProfile <balanced|aggressive|defensive>
static void RunAICoreEvalProfile(const TArray<FString>& Args, UWorld*)
{
    if (Args.Num() < 1) {
        UE_LOG(LogAICore, Log, TEXT("Usage: AICore.EvalProfile <balanced|aggressive|defensive>"));
        return;
    }
    const FString Mode = Args[0].ToLower();

    auto* HP    = IConsoleManager::Get().FindConsoleVariable(TEXT("AICore.W_HP"));
    auto* Pos   = IConsoleManager::Get().FindConsoleVariable(TEXT("AICore.W_Pos"));
    auto* TF    = IConsoleManager::Get().FindConsoleVariable(TEXT("AICore.W_ThreatFor"));
    auto* TA    = IConsoleManager::Get().FindConsoleVariable(TEXT("AICore.W_ThreatAgainst"));
    auto* Coh   = IConsoleManager::Get().FindConsoleVariable(TEXT("AICore.W_Coh"));

    if (Mode == TEXT("balanced")) {
        HP->Set(100); Pos->Set(3); TF->Set(25); TA->Set(35); Coh->Set(2);
        UE_LOG(LogAICore, Log, TEXT("[EvalProfile] balanced"));
    }
    else if (Mode == TEXT("aggressive")) {
        HP->Set(100); Pos->Set(5); TF->Set(40); TA->Set(25); Coh->Set(1);
        UE_LOG(LogAICore, Log, TEXT("[EvalProfile] aggressive"));
    }
    else if (Mode == TEXT("defensive")) {
        HP->Set(100); Pos->Set(2); TF->Set(20); TA->Set(45); Coh->Set(3);
        UE_LOG(LogAICore, Log, TEXT("[EvalProfile] defensive"));
    }
    else {
        UE_LOG(LogAICore, Warning, TEXT("[EvalProfile] Unknown mode: %s"), *Mode);
    }
}
static FAutoConsoleCommandWithWorldAndArgs CmdAICoreEvalProfile(
    TEXT("AICore.EvalProfile"),
    TEXT("Usage: AICore.EvalProfile <balanced|aggressive|defensive>"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICoreEvalProfile)
);

// AICore.SearchWorld [softMs] [hardMs] [maxDepth] [W] [H] [side=0] [teamAP=5]
static void RunAICoreSearchWorld(const TArray<FString>& Args, UWorld* World)
{
    EnsureTTValidityOnWeightsChange();

    int32 SoftMs = GAICoreDefaultSoftMs, HardMs = GAICoreDefaultHardMs, MaxDepth = GAICoreDefaultDepth;
    int32 W = 5, H = 5, Side = 0, TeamAP = 5;
    if (Args.Num() >= 1) LexFromString(SoftMs, *Args[0]);
    if (Args.Num() >= 2) LexFromString(HardMs, *Args[1]);
    if (Args.Num() >= 3) LexFromString(MaxDepth, *Args[2]);
    if (Args.Num() >= 4) LexFromString(W, *Args[3]);
    if (Args.Num() >= 5) LexFromString(H, *Args[4]);
    if (Args.Num() >= 6) LexFromString(Side, *Args[5]);
    if (Args.Num() >= 7) LexFromString(TeamAP, *Args[6]);

    FSnapshotBuildConfig Cfg; Cfg.Width = W; Cfg.Height = H; Cfg.SideToAct = Side; Cfg.TeamAPStart = TeamAP;

    GameState S; FString Info;
    if (!AICore::BuildSnapshotFromWorld(World, Cfg, S, &Info)) {
        UE_LOG(LogAICore, Error, TEXT("[SearchWorld] Snapshot build failed."));
        return;
    }
    UE_LOG(LogAICore, Log, TEXT("[SearchWorld] %s"), *Info);

    SearchParams P{};
    P.Budget.SoftMs = SoftMs; P.Budget.HardMs = HardMs;
    P.MaxDepth = MaxDepth; P.RootK = GAICoreDefaultRootK; P.NodeK = GAICoreDefaultNodeK;

    // CVars 스냅샷
    P.E.HP = CVarAICore_W_HP.GetValueOnAnyThread();
    P.E.Pos = CVarAICore_W_Pos.GetValueOnAnyThread();
    P.E.TFor = CVarAICore_W_ThreatFor.GetValueOnAnyThread();
    P.E.TAgainst = CVarAICore_W_ThreatAgainst.GetValueOnAnyThread();
    P.E.Coh = CVarAICore_W_Cohesion.GetValueOnAnyThread();
    P.O.Pos = CVarAICore_OrderPos.GetValueOnAnyThread();
    P.O.Threat = CVarAICore_OrderThreat.GetValueOnAnyThread();
    P.QStrict = (CVarAICore_QStrict.GetValueOnAnyThread() != 0);
    P.Dedup = (CVarAICore_Dedup.GetValueOnAnyThread() != 0);
    P.Epsilon = CVarAICore_Epsilon.GetValueOnAnyThread();
    P.NoiseSeed = CVarAICore_NoiseSeed.GetValueOnAnyThread();

    BasicRules R; // S2에서 실제 규칙 어댑터로 교체
    std::vector<Action> PV; int score = 0; int64 nodes = 0; double ms = 0.0;
    SearchRoot_IDDFS(S, R, P, PV, score, nodes, ms);

    FString pv;
    for (size_t i = 0; i < PV.size(); ++i) {
        const Action& a = PV[i];
        if (a.type == ActionType::Move)        pv += FString::Printf(TEXT("Move(u=%d->%d)"), a.actorId, a.tileIndex);
        else if (a.type == ActionType::Attack) pv += FString::Printf(TEXT("Attack(u=%d->t=%d)"), a.actorId, a.targetId);
        else                                  pv += FString::Printf(TEXT("Pass(u=%d)"), a.actorId);
        if (i + 1 < PV.size()) pv += TEXT(" -> ");
    }

    const double nps = (ms > 0.0) ? (double)nodes / (ms / 1000.0) : 0.0;
    UE_LOG(LogAICore, Log, TEXT("[SearchWorld] bestScore=%d depth<=%d nodes=%lld time=%.2fms nps=%.0f"),
        score, MaxDepth, (long long)nodes, ms, nps);
    UE_LOG(LogAICore, Log, TEXT("[SearchWorld] PV: %s"), *pv);

    if (CVarAICore_Overlay.GetValueOnAnyThread() != 0 && GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green,
            FString::Printf(TEXT("[AICore] depth<=%d score=%d nodes=%lld time=%.2fms nps=%.0f"),
                MaxDepth, score, (long long)nodes, ms, nps));
        GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Silver, FString::Printf(TEXT("[PV] %s"), *pv));
    }
}

static FAutoConsoleCommandWithWorldAndArgs CmdAICoreSearchWorld(
    TEXT("AICore.SearchWorld"),
    TEXT("Usage: AICore.SearchWorld [softMs] [hardMs] [maxDepth] [W] [H] [side=0] [teamAP=5]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICoreSearchWorld)
);

// AICore.SearchWorldUTBG [soft] [hard] [depth] [W] [H] [side=0] [teamAP=5]
static void RunAICoreSearchWorldUTBG(const TArray<FString>& Args, UWorld* World)
{
    // 1) 스냅샷
    int32 Soft = GAICoreDefaultSoftMs, Hard = GAICoreDefaultHardMs, D = GAICoreDefaultDepth;
    int32 W = 10, H = 10, Side = 0, TeamAP = 5;
    if (Args.Num() >= 1) LexFromString(Soft, *Args[0]);
    if (Args.Num() >= 2) LexFromString(Hard, *Args[1]);
    if (Args.Num() >= 3) LexFromString(D, *Args[2]);
    if (Args.Num() >= 4) LexFromString(W, *Args[3]);
    if (Args.Num() >= 5) LexFromString(H, *Args[4]);
    if (Args.Num() >= 6) LexFromString(Side, *Args[5]);
    if (Args.Num() >= 7) LexFromString(TeamAP, *Args[6]);

    FSnapshotBuildConfig Cfg; Cfg.Width = W; Cfg.Height = H; Cfg.SideToAct = Side; Cfg.TeamAPStart = TeamAP;

    GameState S; FString Info;
    if (!AICore::BuildSnapshotFromWorld(World, Cfg, S, &Info)) {
        UE_LOG(LogAICore, Error, TEXT("[SearchWorldUTBG] snapshot failed."));
        return;
    }
    UE_LOG(LogAICore, Log, TEXT("[SearchWorldUTBG] %s"), *Info);

    // 2) 규칙/탐색
    UTBGRules R; R.TurnAP = TeamAP; // 한 턴 AP
    FTimeBudget B{ Soft, Hard };
    FTimeManager TM; TM.Start(B);
    if (!GAICoreTT.IsReady()) { GAICoreTT.ResizeMB(64); }
    SearchCtxUTBG Ctx; Ctx.TM = &TM; Ctx.TT = &GAICoreTT;

    std::vector<Action> PV; int score = 0;
    double ms = 0.0;

    // IDDFS
    int best = std::numeric_limits<int>::min();
    std::vector<Action> bestPV;
    const double T0 = FPlatformTime::Seconds();

    for (int depth = 1; depth <= D; ++depth)
    {
        if (TM.SoftExpired()) break;

        int iterBest = std::numeric_limits<int>::min();
        std::vector<Action> iterPV;

        std::vector<Action> root;
        R.generateLegal(S, root);
        AICore::SortActionsDeterministic(
            S, root,
            AICore::OrderWeights{
                CVarAICore_OrderPos.GetValueOnAnyThread(),
                CVarAICore_OrderThreat.GetValueOnAnyThread()
            },
            /*attackDamage*/5);

        for (const auto& a : root)
        {
            if (TM.SoftExpired()) break;
            FScopedMakeT<UTBGRules, UTBGDelta> guard(R, S, a);

            std::vector<Action> childPV;
            const int sc = -AlphaBeta_UTBG(S, depth - 1, -1000000000, +1000000000, R, Ctx, childPV);

            if (sc > iterBest ||
                (sc == iterBest && a.signature() < (iterPV.empty() ? ~0ULL : iterPV.front().signature())))
            {
                iterBest = sc;
                iterPV.clear(); iterPV.push_back(a);
                iterPV.insert(iterPV.end(), childPV.begin(), childPV.end());
            }
        }

        if (!iterPV.empty()) { best = iterBest; bestPV = iterPV; }
    }

    ms = (FPlatformTime::Seconds() - T0) * 1000.0;
    PV = bestPV; score = best;

    // 3) 출력
    FString pvText;
    for (size_t i = 0; i < PV.size(); ++i) {
        const Action& a = PV[i];
        if (a.type == ActionType::Move)        pvText += FString::Printf(TEXT("Move(u=%d->%d)"), a.actorId, a.tileIndex);
        else if (a.type == ActionType::Attack) pvText += FString::Printf(TEXT("Attack(u=%d->t=%d)"), a.actorId, a.targetId);
        else if (a.type == ActionType::EndTurn)pvText += TEXT("EndTurn");
        else                                    pvText += FString::Printf(TEXT("Pass(u=%d)"), a.actorId);
        if (i + 1 < PV.size()) pvText += TEXT(" -> ");
    }

    const double nps = (ms > 0.0) ? (double)Ctx.Nodes / (ms / 1000.0) : 0.0;
    UE_LOG(LogAICore, Log, TEXT("[SearchWorldUTBG] bestScore=%d depth<=%d nodes=%lld time=%.2fms nps=%.0f"),
        score, D, (long long)Ctx.Nodes, ms, nps);
    UE_LOG(LogAICore, Log, TEXT("[SearchWorldUTBG] PV: %s"), *pvText);

    if (CVarAICore_Overlay.GetValueOnAnyThread() != 0 && GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
            FString::Printf(TEXT("[AICore]UTBG depth<=%d score=%d nodes=%lld time=%.2fms nps=%.0f"),
                D, score, (long long)Ctx.Nodes, ms, nps));
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Silver,
            FString::Printf(TEXT("[PV] %s"), *pvText));
    }
    WriteUTBGSearchLogJSONL(S, R, PV, score, D, Ctx.Nodes, ms);
}

static FAutoConsoleCommandWithWorldAndArgs CmdAICoreSearchWorldUTBG(
    TEXT("AICore.SearchWorldUTBG"),
    TEXT("Usage: AICore.SearchWorldUTBG [soft] [hard] [depth] [W] [H] [side=0] [teamAP=5]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICoreSearchWorldUTBG)
);