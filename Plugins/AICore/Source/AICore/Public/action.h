#pragma once
#include <cstdint>

enum class ActionType : uint8_t { Move, Attack, Skill, Pass, EndTurn };

struct Action {
    int actorId = -1;
    ActionType type = ActionType::Pass;
    uint8_t apCost = 0;
    int targetId = -1;   // 공격 타깃 유닛
    int tileIndex = -1;  // 이동/스킬 타일
    uint8_t skillId = 0;

    uint64_t signature() const {
        return ((uint64_t)type << 60)
            | (((uint64_t)actorId & 0xFFFFF) << 40)
            | (((uint64_t)targetId & 0xFFFFF) << 20)
            | ((uint64_t)(tileIndex & 0xFFFF))
            | (((uint64_t)apCost) << 56)
            | (((uint64_t)skillId) << 48);
    }
};