#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include "zobrist.h"
#include "action.h"

struct Unit {
    int  id = -1;
    int  team = 0;      // 0=����, 1=���
    int  tile = -1;     // 0..W*H-1 (���� W = H = 8)
    int  hp = 10;
    int  ap = 2;        // ���� AP -> team ap�� ������ ����
    bool alive = true;
    int  attack = 5;
};


struct Delta {
    uint64_t prevZ = 0;

    int   actorId = -1;
    int   prevActorTile = -1;
    int   prevActorAP = -1;     // ���� AP -> team ap�� ������ ����
    int   prevActorHP = -1;
    bool  prevActorAlive = true;

    int   targetId = -1;
    int   prevTargetHP = -1;
    bool  prevTargetAlive = true;

    bool  changedPos = false;
    bool  changedAP = false;    // ���� AP -> team ap�� ������ ����
    bool  targetChangedHP = false;
    bool  targetChangedAlive = false;
};

struct GameState {
    int width = 0, height = 0;
    int sideToAct = 0;     // 0 or 1
    int maxAP = 5;
    std::vector<Unit> units;

    int teamAP[2] = { 0, 0 };

    Zobrist Z;
    uint64_t key = 0;
    std::vector<Delta> stack;

    int boardSize() const { return width * height; }

    // �߿�: teamAP ��ū XOR ����
    inline void xorTeamAP(int side, int ap) {
        if (Z.maxAP > 0) {
            if (ap < 0) ap = 0;
            if (ap > Z.maxAP) ap = Z.maxAP;
            key ^= Z.teamAP[Z.idxTeamAP(side, ap)];
        }
    }

    // �߿�: Zobrist �ʱ�ȭ (teamAP ����)
    inline void initZobrist(uint64_t seed, int maxUnits) {
        // teamAP[]�� ���� ä���� ���·� ȣ��Ǿ�� ��(������ ���� ����)
        const int inferredMaxAP = std::max(teamAP[0], teamAP[1]);  // ������ �� ���
        const int zMaxAP = std::max(inferredMaxAP, maxAP);         // �� �� ū ��
        Z.init(seed, maxUnits, boardSize(), zMaxAP);

        key = 0;
        // side
        key ^= Z.sideToAct[sideToAct];

        // unit positions
        for (const auto& u : units) {
            if (u.alive && u.tile >= 0) {
                key ^= Z.unitPos[Z.idxUnitPos(u.id, u.tile)];
            }
        }

        // teamAP (���� ��� XOR)
        xorTeamAP(0, teamAP[0]);
        xorTeamAP(1, teamAP[1]);
    }

    // ���� ����: ��ġ/HP/������(��/��AP/���̵� ����� �꿡��!)
    void make(const Action& a, Delta& d) {
        d = {};
        d.prevZ = key;
        d.actorId = a.actorId;
        
        if (a.actorId >= 0) {
            auto& A = units[a.actorId];
            d.prevActorTile = A.tile;
            d.prevActorAP = A.ap;       // ����
            d.prevActorHP = A.hp;
            d.prevActorAlive = A.alive;

            // ����
            if (a.apCost > 0) { A.ap -= a.apCost; d.changedAP = true; }
        }

        if (a.type == ActionType::Move && d.actorId >= 0) {
            auto& A = units[d.actorId];
            if (A.alive && A.tile >= 0) {
                key ^= Z.unitPos[Z.idxUnitPos(A.id, A.tile)];
                A.tile = a.tileIndex;
                d.changedPos = true;
                key ^= Z.unitPos[Z.idxUnitPos(A.id, A.tile)];
            }
        }
        else if (a.type == ActionType::Attack && a.targetId >= 0) {
            auto& T = units[a.targetId];
            d.targetId = a.targetId;
            d.prevTargetHP = T.hp;
            d.prevTargetAlive = T.alive;

            const int dmg = (a.actorId >= 0 && a.actorId < (int)units.size() && units[a.actorId].attack > 0) ? units[a.actorId].attack : 5;
            T.hp -= dmg;
            d.targetChangedHP = true;

            if (T.hp <= 0 && T.alive) {
                T.alive = false;
                d.targetChangedAlive = true;
                if (T.tile >= 0) {
                    key ^= Z.unitPos[Z.idxUnitPos(T.id, T.tile)];
                }
            }
        }
        // EndTurn/Pass�� ���⼭�� ���� ó���� �� ����(�� ��ȯ/�� AP�� Rules���� ó��)
        stack.push_back(d);
    }

    inline void unmake(const Delta& d) {
        // NOTE: teamAP/�� ��ȯ�� UTBGRules::unmake()�� ���� �ǵ��� ��,
        // ���⿡�� ����/HP/�����Ǹ� �ǵ����ϴ�. (Ű�� prevZ�� ����)
        if (d.actorId >= 0) {
            auto& A = units[d.actorId];
            A.tile = d.prevActorTile;
            A.ap = d.prevActorAP;       // ����
            A.hp = d.prevActorHP;
            A.alive = d.prevActorAlive;
        }
        if (d.targetId >= 0) {
            auto& T = units[d.targetId];
            T.hp = d.prevTargetHP;
            T.alive = d.prevTargetAlive;
        }
        key = d.prevZ; // ��ü Ű ����
    }
};

