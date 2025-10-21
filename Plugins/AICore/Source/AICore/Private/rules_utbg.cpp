#include "rules_utbg.h"
#include "state.h"

void UTBGRules::generateLegal(const GameState& S, std::vector<Action>& out) const
{
    out.clear();
    const int side = S.sideToAct;
    const int pool = S.teamAP[side];
    const int W = S.width, H = S.height;

    auto inBounds = [&](int x, int y) { return x >= 0 && y >= 0 && x < W && y < H; };
    auto occupied = [&](int tile)->bool {
        for (const auto& u : S.units) if (u.alive && u.tile == tile) return true;
        return false;
        };

    for (const auto& u : S.units)
    {
        if (!u.alive || u.team != side) continue;

        const int x = u.tile % W, y = u.tile / W;

        // Move (코스트 체크)
        if (pool >= MoveCost)
        {
            const int dx[4] = { +1,-1,0,0 }, dy[4] = { 0,0,+1,-1 };
            for (int d = 0; d < 4; ++d)
            {
                const int nx = x + dx[d], ny = y + dy[d];
                if (!inBounds(nx, ny)) continue;
                const int nt = ny * W + nx;
                if (occupied(nt)) continue;

                Action a; a.actorId = u.id; a.type = ActionType::Move; a.tileIndex = nt; a.apCost = MoveCost;
                out.push_back(a);
            }
        }

        // Attack (인접만, 코스트 체크)
        if (pool >= AttackCost)
        {
            for (const auto& v : S.units)
            {
                if (!v.alive || v.team == u.team) continue;
                const int vx = v.tile % W, vy = v.tile / W;
                const int manhattan = FMath::Abs(x - vx) + FMath::Abs(y - vy);
                if (manhattan == 1)
                {
                    Action a; a.actorId = u.id; a.type = ActionType::Attack; a.targetId = v.id; a.apCost = AttackCost;
                    out.push_back(a);
                }
            }
        }
    }

    // 항상 EndTurn 1개 추가 (actorId는 사용하지 않음)
    {
        Action e; e.actorId = -1; e.type = ActionType::EndTurn; e.apCost = 0;
        out.push_back(e);
    }
}

void UTBGRules::FlipSideInPlace(GameState& S)
{
    S.key ^= S.Z.sideToAct[S.sideToAct];
    S.sideToAct ^= 1;
    S.key ^= S.Z.sideToAct[S.sideToAct];
}

void UTBGRules::make(GameState& S, const Action& a, Delta& d) const
{
    UTBGDelta& dx = static_cast<UTBGDelta&>(d);
    dx.SideBefore = (uint8)S.sideToAct;
    dx.APBefore[0] = (int16)S.teamAP[0];
    dx.APBefore[1] = (int16)S.teamAP[1];
    dx.bFlippedTurn = 0;

    // 1) 원자 전이(위치/HP/생존)
    S.make(a, d);

    // 2) 팀 AP/턴 처리 (+Zobrist teamAP 토큰)
    const int side = dx.SideBefore;

    if (a.type == ActionType::EndTurn)
    {
        dx.bFlippedTurn = 1;
        FlipSideInPlace(S);

        if (S.Z.maxAP > 0)
        {
            S.xorTeamAP(S.sideToAct, S.teamAP[S.sideToAct]); // (보통 0) XOR-out
            S.teamAP[S.sideToAct] = TurnAP;
            S.xorTeamAP(S.sideToAct, S.teamAP[S.sideToAct]); // 새 AP XOR-in
        }
        else
        {
            S.teamAP[S.sideToAct] = TurnAP;
        }
    }
    else
    {
        if (S.Z.maxAP > 0)
        {
            S.xorTeamAP(side, S.teamAP[side]);                          // old XOR-out
            S.teamAP[side] = FMath::Max(0, S.teamAP[side] - a.apCost);
            S.xorTeamAP(side, S.teamAP[side]);                          // new XOR-in
        }
        else
        {
            S.teamAP[side] = FMath::Max(0, S.teamAP[side] - a.apCost);
        }

        if (S.teamAP[side] == 0)
        {
            dx.bFlippedTurn = 1;
            FlipSideInPlace(S);

            if (S.Z.maxAP > 0)
            {
                S.xorTeamAP(S.sideToAct, S.teamAP[S.sideToAct]);        // (보통 0) XOR-out
                S.teamAP[S.sideToAct] = TurnAP;
                S.xorTeamAP(S.sideToAct, S.teamAP[S.sideToAct]);        // 새 AP XOR-in
            }
            else
            {
                S.teamAP[S.sideToAct] = TurnAP;
            }
        }
    }
}

void UTBGRules::unmake(GameState& S, const Delta& d) const
{
    const UTBGDelta& dx = static_cast<const UTBGDelta&>(d);

    // 1) 턴 전환을 했었다면 먼저 sideToAct를 원복
    if (dx.bFlippedTurn)
        FlipSideInPlace(S);

    // 2) 팀 AP 원복(양쪽) + 키 원복
    if (S.Z.maxAP > 0)
    {
        S.xorTeamAP(0, S.teamAP[0]);   // 현재값 XOR-out
        S.xorTeamAP(1, S.teamAP[1]);
        S.teamAP[0] = dx.APBefore[0];  // 값 복구
        S.teamAP[1] = dx.APBefore[1];
        S.xorTeamAP(0, S.teamAP[0]);   // 이전값 XOR-in
        S.xorTeamAP(1, S.teamAP[1]);
    }
    else
    {
        S.teamAP[0] = dx.APBefore[0];
        S.teamAP[1] = dx.APBefore[1];
    }

    // 3) 유닛/HP/포지션 등 기본 상태 롤백(key는 prevZ로)
    S.unmake(d);
}