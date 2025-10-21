#include "AICoreSnapshot.h"
#include "AICoreLog.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"

// 유틸: 세션 임시 UnitId 매핑
namespace {
    TMap<TWeakObjectPtr<APawn>, uint16> GUnitIdMap;
    uint16 GNextUnitId = 0;

    uint16 GetOrAssignUnitId(APawn* P)
    {
        if (const uint16* Found = GUnitIdMap.Find(P)) return *Found;
        const uint16 NewId = GNextUnitId++;
        GUnitIdMap.Add(P, NewId);
        return NewId;
    }

    // FProperty 리플렉션 유틸
    static bool TryGetInt(const UObject* Obj, FName Name, int32& Out)
    {
        if (!Obj) return false;
        if (const FProperty* Prop = Obj->GetClass()->FindPropertyByName(Name))
        {
            if (const FIntProperty* IP = CastField<FIntProperty>(Prop))
            {
                Out = IP->GetPropertyValue_InContainer(Obj);
                return true;
            }
        }
        return false;
    }

    static bool TryGetFIntPoint(const UObject* Obj, FName Name, FIntPoint& Out)
    {
        if (!Obj) return false;
        if (const FProperty* Prop = Obj->GetClass()->FindPropertyByName(Name))
        {
            if (const FStructProperty* SP = CastField<FStructProperty>(Prop))
            {
                if (SP->Struct == TBaseStructure<FIntPoint>::Get())
                {
                    Out = *SP->ContainerPtrToValuePtr<FIntPoint>(Obj);
                    return true;
                }
            }
        }
        return false;
    }

    static bool TryGetTeamFromEnum(const UObject* Obj, int& OutTeamIdx, bool& OutNoTeam)
    {
        OutNoTeam = false;
        if (!Obj) return false;

        if (const FProperty* Prop = Obj->GetClass()->FindPropertyByName(TEXT("Team")))
        {
            // ETeam 이면 보통 FEnumProperty(스코프 enum) 또는 FByteProperty(언더라잉 uint8) 두 경우
            if (const FEnumProperty* EP = CastField<FEnumProperty>(Prop))
            {
                const void* Ptr = EP->ContainerPtrToValuePtr<void>(Obj);
                const int64 Raw = EP->GetUnderlyingProperty()->GetSignedIntPropertyValue(Ptr);
                if (UEnum* E = EP->GetEnum())
                {
                    const FString NameStr = E->GetNameStringByValue(Raw);
                    if (NameStr.Contains(TEXT("NoTeam"))) { OutNoTeam = true; return true; }
                    if (NameStr.Contains(TEXT("Blue"))) { OutTeamIdx = 0; return true; }
                    if (NameStr.Contains(TEXT("Red"))) { OutTeamIdx = 1; return true; }
                }
                // 이름 못 찾으면 parity로라도 분리
                OutTeamIdx = (int)(Raw % 2);
                return true;
            }
            if (const FByteProperty* BP = CastField<FByteProperty>(Prop))
            {
                const uint8* ValPtr = BP->ContainerPtrToValuePtr<uint8>(Obj);
                const uint8 Raw = *ValPtr;
                // 0=NoTeam 으로 가정(일반적), 1/2는 Blue/Red 가정
                if (Raw == 0) { OutNoTeam = true; return true; }
                OutTeamIdx = (Raw == 2) ? 1 : 0; // 1->Blue(0), 2->Red(1) 가정
                return true;
            }
        }
        return false;
    }
}

void AICore::ResetSnapshotUnitIds()
{
    GUnitIdMap.Reset();
    GNextUnitId = 0;
}

bool AICore::BuildSnapshotFromWorld(UWorld* World, const FSnapshotBuildConfig& Cfg,
    GameState& Out, FString* OutDebugInfo)
{
    if (!World) return false;

    Out = GameState{};
    Out.width = Cfg.Width;
    Out.height = Cfg.Height;
    Out.sideToAct = Cfg.SideToAct;
    Out.units.clear();

    int32 Count = 0;

    // 현재 레벨의 Pawn 전부 스캔(프로젝트 PawnBase 포함)
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* P = *It;
        if (!IsValid(P)) continue;

        // 1) GridCoord가 있어야 "유닛"으로 취급 (카메라 등 제외)
        FIntPoint Grid(-1, -1);
        if (!TryGetFIntPoint(P, TEXT("GridCoord"), Grid)) continue;
        if (Grid.X < 0 || Grid.Y < 0) continue;

        // 2) Team enum 읽기 (NoTeam이면 제외)
        int teamIdx = 0; bool bNoTeam = false;
        if (!TryGetTeamFromEnum(P, teamIdx, bNoTeam)) {
            continue; // 팀 속성이 없다면 스냅샷에서 제외(명시적)
        }
        if (bNoTeam) continue; // ET_NoTeam 은 스킵

        // 3) HP
        int32 CurHP = 0, MaxHP = 0;
        const bool bHasHP = TryGetInt(P, TEXT("CurrentHP"), CurHP);
        const bool bHasMaxHP = TryGetInt(P, TEXT("MaxHP"), MaxHP);
        const bool bAlive = !bHasHP ? true : (CurHP > 0);
        if (Cfg.bOnlyLiving && !bAlive) continue;

        // 4) 타일 인덱스
        const int tile = Grid.Y * Cfg.Width + Grid.X;

        // 5) ID / HP / (임시 AP 스텁)
        const uint16 id = GetOrAssignUnitId(P);
        const int hpForSnapshot = bHasHP ? CurHP : (bHasMaxHP ? MaxHP : 10);
        const int apStub = Cfg.FallbackUnitAP; // S2에서 제거 예정

        // 6) Attack
        int32 Attack = 0;
        bool bHasAttack =
            TryGetInt(P, TEXT("AttackPower"), Attack)   ||
            TryGetInt(P, TEXT("Attack"), Attack)        ||
            TryGetInt(P, TEXT("BaseAttack"), Attack);

        if (!bHasAttack || Attack <= 0) Attack = 5;

        Out.units.push_back(Unit{ id, teamIdx, tile, hpForSnapshot, apStub, bAlive, Attack });
        ++Count;
    }

    // 팀 AP 풀(현재 턴 팀만 보유)
    Out.teamAP[0] = 0;
    Out.teamAP[1] = 0;
    Out.teamAP[Cfg.SideToAct] = Cfg.TeamAPStart;
    Out.maxAP = std::max(Out.teamAP[0], Out.teamAP[1]);

    // Zobrist 테이블 준비
    Out.initZobrist(Cfg.ZobristSeed, (int)Out.units.size());

    if (OutDebugInfo)
    {
        *OutDebugInfo = FString::Printf(TEXT("units=%d WxH=%dx%d side=%d teamAP=[%d,%d]"),
            Count, Cfg.Width, Cfg.Height, Cfg.SideToAct, Out.teamAP[0], Out.teamAP[1]);
    }
    return true;
}