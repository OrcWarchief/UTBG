#pragma once
#include "rules.h"
#include "state.h"
// UTBG�� ��Ÿ: team AP/side ������ ����
struct UTBGDelta : public Delta
{
    int16  APBefore[2] = { 0,0 };
    uint8  SideBefore = 0;
    uint8  bFlippedTurn = 0; // make���� ���� �Ѱ����
    int prevTileActor = -1;
    int prevTileTarget = -1;
    int prevHPActor = -1;
    int prevHPTarget = -1;
    bool targetWasAlive = true;

    int prevTeamAP[2] = { 0, 0 };

    bool bFlippedSide = false;
};

// �� AP Ǯ ��� ��Ģ
struct UTBGRules
{
    // �ڽ�Ʈ & �� �� ���� AP (UTBG �⺻ 5)
    int MoveCost = 1;
    int AttackCost = 1;
    int SkillCost = 1;
    int TurnAP = 5;

    // �չ� �� ���� (teamAP ����)
    void generateLegal(const GameState& S, std::vector<Action>& out) const;

    // ���� ����/�ǵ����� (teamAP ����/����ȯ ����)
    void make(GameState& S, const Action& a, Delta& d) const;
    void unmake(GameState& S, const Delta& d) const;

private:
    // ���� ����: ���̵� ��ȯ
    static void FlipSideInPlace(GameState& S);
};