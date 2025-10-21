#pragma once
#include "CoreMinimal.h"
#include "state.h"

class UWorld;
class APawnBase;
class ABoard;

struct FSnapshotBuildConfig
{
    int32  Width = 8;
    int32  Height = 8;
    int32  SideToAct = 0;
    bool   bOnlyLiving = true;
    int32  TeamAPStart = 5;
    // (임시) S2 전까지는 기존 규칙/탐색기가 "유닛 AP>0"을 바라보므로
    // 합법수 필터에 안 걸리게 스텁 AP를 채워줍니다.
    int32  FallbackUnitAP = 2;
    uint64 ZobristSeed = 0xC0FFEEULL;     // 키 생성용 시드
};

namespace AICore
{
    // 월드에서 FGameState 스냅샷을 구축. 실패 시 false.
    bool BuildSnapshotFromWorld(UWorld* World, const FSnapshotBuildConfig& Cfg,
        GameState& Out, FString* OutDebugInfo = nullptr);

    // 세션 내 임시 UnitId 매핑 초기화
    void ResetSnapshotUnitIds();
}

const TArray<TWeakObjectPtr<APawnBase>>& GetLastUnitLUT();
ABoard* GetLastBoard();