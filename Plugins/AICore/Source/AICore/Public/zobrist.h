#pragma once
#include <vector>
#include <cstdint>
#include "rng.h"

struct Zobrist {
    int maxUnits = 0, boardSize = 0, maxAP = 0;
    std::vector<uint64_t> sideToAct;  // [2]
    std::vector<uint64_t> unitPos;    // [maxUnits * boardSize]
    std::vector<uint64_t> teamAP;     // [2 * (maxAP+1)]

    void init(uint64_t seed, int maxUnits_, int boardSize_, int maxAP_) {
        maxUnits = maxUnits_; boardSize = boardSize_; maxAP = maxAP_;
        sideToAct.resize(2);
        unitPos.resize((size_t)maxUnits * boardSize);
        teamAP.resize(2ull * (maxAP + 1));

        SplitMix64 rng(seed);
        for (int i = 0; i < 2; ++i) sideToAct[i] = rng.next();
        for (size_t i = 0; i < unitPos.size(); ++i) unitPos[i] = rng.next();
        for (size_t i = 0; i < teamAP.size(); ++i) teamAP[i] = rng.next();
    }
    inline size_t idxUnitPos(int unitId, int tile) const {
        return (size_t)unitId * boardSize + tile;
    }
    inline size_t idxTeamAP(int side, int ap) const {
        if (ap < 0) ap = 0; if (ap > maxAP) ap = maxAP;
        return (size_t)side * (maxAP + 1) + (size_t)ap;
    }
};