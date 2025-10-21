#include "CoreMinimal.h"
#include "AICoreLog.h"
#include "HAL/IConsoleManager.h"
#include "String/LexFromString.h"
#include "Engine/World.h"  
#include "rules.h"
#include "rng.h"
#include <algorithm>
#include <vector>
#include <limits>

static int Manhattan(int a, int b, int W) {
    int ax = a % W, ay = a / W, bx = b % W, by = b / W;
    return FMath::Abs(ax - bx) + FMath::Abs(ay - by);
}

static int NearestEnemyDistFrom(const GameState& S, int tile, int myTeam) {
    int best = INT_MAX;
    for (const auto& e : S.units) {
        if (!e.alive || e.team == myTeam || e.tile < 0) continue;
        best = FMath::Min(best, Manhattan(tile, e.tile, S.width));
    }
    return (best == INT_MAX) ? 0 : best;
}

// 간단 점수: 확정킬(5000) > 피해량 + 이동 시 적에게 가까워지면 가점 > 패스는 소폭 감점
static int ScoreAction(const GameState& S, const Action& a)
{
    int sc = 0;
    if (a.type == ActionType::Attack && a.targetId >= 0) {
        const auto& t = S.units[a.targetId];
        const int dmg = 5; // state.make와 동일 값
        int remaining = t.hp - dmg;
        if (remaining <= 0) sc += 5000;        // 확정 킬
        sc += FMath::Clamp(dmg, 0, t.hp) * 10; // 피해량 가중
    }
    else if (a.type == ActionType::Move && a.tileIndex >= 0) {
        const auto& u = S.units[a.actorId];
        int before = NearestEnemyDistFrom(S, u.tile, u.team);
        int after = NearestEnemyDistFrom(S, a.tileIndex, u.team);
        sc += (before - after) * 8; // 적에게 가까워질수록 +
    }
    else if (a.type == ActionType::Pass) {
        sc -= 5; // 소폭 페널티(그래도 AP 정리 용도로 사용할 수 있게)
    }

    // 동점 타이브레이크(결정성): signature 하위비트
    sc = (sc << 1) | int(a.signature() & 1ULL);
    return sc;
}

static void SortActionsDeterministic(const GameState& S, std::vector<Action>& moves)
{
    std::stable_sort(moves.begin(), moves.end(),
        [&](const Action& a, const Action& b) {
            const int sa = ScoreAction(S, a), sb = ScoreAction(S, b);
            if (sa != sb) return sa > sb;
            return a.signature() < b.signature();
        });
}

static void DecideGreedy(GameState& S, BasicRules& R, int maxLen, TArray<FString>& OutHumanReadable)
{
    std::vector<Delta> deltas; deltas.reserve(maxLen);
    int taken = 0;
    const double T0 = FPlatformTime::Seconds();
    double tPrev = T0;

    while (taken < maxLen) {
        std::vector<Action> moves; R.generateLegal(S, moves);
        if (moves.empty()) break;

        // AP 없거나 모자란 액션 제거(안전망)
        moves.erase(std::remove_if(moves.begin(), moves.end(),
            [&](const Action& m) { return S.units[m.actorId].ap < m.apCost; }), moves.end());
        if (moves.empty()) break;

        // ▶ 여기서 공용 정렬을 활용 (아래 2) 참고)
        SortActionsDeterministic(S, moves);
        const Action& best = moves.front();

        Delta d; R.make(S, best, d);
        deltas.push_back(d); taken++;

        // 로그용 텍스트
        if (best.type == ActionType::Move)
            OutHumanReadable.Add(FString::Printf(TEXT("Move(u=%d -> %d)"), best.actorId, best.tileIndex));
        else if (best.type == ActionType::Attack)
            OutHumanReadable.Add(FString::Printf(TEXT("Attack(u=%d -> t=%d)"), best.actorId, best.targetId));
        else
            OutHumanReadable.Add(FString::Printf(TEXT("Pass(u=%d)"), best.actorId));

        // (선택) 스텝별 소요 시간
        const double stepMs = (FPlatformTime::Seconds() - tPrev) * 1000.0;
        UE_LOG(LogAICore, Verbose, TEXT("[Greedy] step=%d, stepTime=%.2fms"), taken, stepMs);
        tPrev = FPlatformTime::Seconds();
    }

    // 총 소요 시간
    const double totalMs = (FPlatformTime::Seconds() - T0) * 1000.0;
    UE_LOG(LogAICore, Log, TEXT("[Greedy] len=%d  total=%.2fms"), taken, totalMs);

    // 상태 복원
    for (int i = (int)deltas.size() - 1; i >= 0; --i) R.unmake(S, deltas[i]);
}

// 콘솔: AICore.Greedy <maxLen=3>
static void RunAICoreGreedy(const TArray<FString>& Args, UWorld* /*World*/)
{
    int32 MaxLen = 3;
    if (Args.Num() >= 1) LexFromString(MaxLen, *Args[0]);

    GameState S;
    S.width = 5; S.height = 5; S.sideToAct = 0;
    S.units = { Unit{0,0,12,10,3,true}, Unit{1,1,13,10,2,true} };
    S.initZobrist(0xC0FFEEULL, (int)S.units.size());

    BasicRules R;
    TArray<FString> seq;
    DecideGreedy(S, R, MaxLen, seq);

    FString joined;
    for (int i = 0; i < seq.Num(); ++i) { joined += seq[i]; if (i + 1 < seq.Num()) joined += TEXT(" -> "); }
    UE_LOG(LogAICore, Log, TEXT("[Greedy] len=%d  %s"), seq.Num(), *joined);
}

static FAutoConsoleCommandWithWorldAndArgs CmdAICoreGreedy(
    TEXT("AICore.Greedy"),
    TEXT("Usage: AICore.Greedy <maxLen=3>  // picks 2-3 step greedy sequence"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunAICoreGreedy)
);