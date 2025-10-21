#pragma once
#include "rules.h"
#include "state.h"
// UTBG용 델타: team AP/side 복원을 포함
struct UTBGDelta : public Delta
{
    int16  APBefore[2] = { 0,0 };
    uint8  SideBefore = 0;
    uint8  bFlippedTurn = 0; // make에서 턴을 넘겼는지
    int prevTileActor = -1;
    int prevTileTarget = -1;
    int prevHPActor = -1;
    int prevHPTarget = -1;
    bool targetWasAlive = true;

    int prevTeamAP[2] = { 0, 0 };

    bool bFlippedSide = false;
};

// 팀 AP 풀 기반 규칙
struct UTBGRules
{
    // 코스트 & 한 턴 시작 AP (UTBG 기본 5)
    int MoveCost = 1;
    int AttackCost = 1;
    int SkillCost = 1;
    int TurnAP = 5;

    // 합법 수 생성 (teamAP 기준)
    void generateLegal(const GameState& S, std::vector<Action>& out) const;

    // 상태 전이/되돌리기 (teamAP 감소/턴전환 포함)
    void make(GameState& S, const Action& a, Delta& d) const;
    void unmake(GameState& S, const Delta& d) const;

private:
    // 내부 보조: 사이드 전환
    static void FlipSideInPlace(GameState& S);
};