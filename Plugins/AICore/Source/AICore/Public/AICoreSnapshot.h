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
    // (�ӽ�) S2 �������� ���� ��Ģ/Ž���Ⱑ "���� AP>0"�� �ٶ󺸹Ƿ�
    // �չ��� ���Ϳ� �� �ɸ��� ���� AP�� ä���ݴϴ�.
    int32  FallbackUnitAP = 2;
    uint64 ZobristSeed = 0xC0FFEEULL;     // Ű ������ �õ�
};

namespace AICore
{
    // ���忡�� FGameState �������� ����. ���� �� false.
    bool BuildSnapshotFromWorld(UWorld* World, const FSnapshotBuildConfig& Cfg,
        GameState& Out, FString* OutDebugInfo = nullptr);

    // ���� �� �ӽ� UnitId ���� �ʱ�ȭ
    void ResetSnapshotUnitIds();
}

const TArray<TWeakObjectPtr<APawnBase>>& GetLastUnitLUT();
ABoard* GetLastBoard();