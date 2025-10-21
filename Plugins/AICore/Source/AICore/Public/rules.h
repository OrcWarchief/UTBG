#pragma once
#include <vector>
#include "state.h"

struct BasicRules {
    int moveAPCost = 1, attackAPCost = 1;

    void generateLegal(const GameState& S, std::vector<Action>& out) const {
        out.clear();
        const int W = S.width, H = S.height;
        auto occupied = [&](int tile)->bool {
            for (const auto& u : S.units) if (u.alive && u.tile == tile) return true;
            return false;
            };
        auto inBounds = [&](int x, int y) { return x >= 0 && y >= 0 && x < W && y < H; };

        for (const auto& u : S.units) {
            if (!u.alive || u.team != S.sideToAct || u.ap < 1) continue;

            // 4방향 이동
            const int x = u.tile % W, y = u.tile / W;
            const int dx[4] = { 1,-1,0,0 }, dy[4] = { 0,0,1,-1 };
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d], ny = y + dy[d];
                if (!inBounds(nx, ny)) continue;
                int nt = ny * W + nx;
                if (occupied(nt)) continue;

                Action p; p.actorId = u.id; p.type = ActionType::Pass; p.apCost = 1;
                out.push_back(p);

                Action a; a.actorId = u.id; a.type = ActionType::Move; a.tileIndex = nt; a.apCost = moveAPCost;
                out.push_back(a);
            }

            // 인접 적 유닛 공격
            for (const auto& v : S.units) {
                if (!v.alive || v.team == u.team) continue;
                int ux = x, uy = y, vx = v.tile % W, vy = v.tile / W;
                int manhattan = (ux > vx ? ux - vx : vx - ux) + (uy > vy ? uy - vy : vy - uy);
                if (manhattan == 1) {
                    Action a; a.actorId = u.id; a.type = ActionType::Attack; a.targetId = v.id; a.apCost = attackAPCost;
                    out.push_back(a);
                }
            }

            // 패스(무료)
            Action p; p.actorId = u.id; p.type = ActionType::Pass; p.apCost = 0;
            out.push_back(p);
        }
    }

    void make(GameState& S, const Action& a, Delta& d) const { S.make(a, d); }
    void unmake(GameState& S, const Delta& d) const { S.unmake(d); }
};