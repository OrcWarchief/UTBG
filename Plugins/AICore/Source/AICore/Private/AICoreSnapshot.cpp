#include "AICoreSnapshot.h"
#include "AICoreLog.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"

// ��ƿ: ���� �ӽ� UnitId ����
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

    // FProperty ���÷��� ��ƿ
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
            // ETeam �̸� ���� FEnumProperty(������ enum) �Ǵ� FByteProperty(������� uint8) �� ���
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
                // �̸� �� ã���� parity�ζ� �и�
                OutTeamIdx = (int)(Raw % 2);
                return true;
            }
            if (const FByteProperty* BP = CastField<FByteProperty>(Prop))
            {
                const uint8* ValPtr = BP->ContainerPtrToValuePtr<uint8>(Obj);
                const uint8 Raw = *ValPtr;
                // 0=NoTeam ���� ����(�Ϲ���), 1/2�� Blue/Red ����
                if (Raw == 0) { OutNoTeam = true; return true; }
                OutTeamIdx = (Raw == 2) ? 1 : 0; // 1->Blue(0), 2->Red(1) ����
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

    // ���� ������ Pawn ���� ��ĵ(������Ʈ PawnBase ����)
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* P = *It;
        if (!IsValid(P)) continue;

        // 1) GridCoord�� �־�� "����"���� ��� (ī�޶� �� ����)
        FIntPoint Grid(-1, -1);
        if (!TryGetFIntPoint(P, TEXT("GridCoord"), Grid)) continue;
        if (Grid.X < 0 || Grid.Y < 0) continue;

        // 2) Team enum �б� (NoTeam�̸� ����)
        int teamIdx = 0; bool bNoTeam = false;
        if (!TryGetTeamFromEnum(P, teamIdx, bNoTeam)) {
            continue; // �� �Ӽ��� ���ٸ� ���������� ����(�����)
        }
        if (bNoTeam) continue; // ET_NoTeam �� ��ŵ

        // 3) HP
        int32 CurHP = 0, MaxHP = 0;
        const bool bHasHP = TryGetInt(P, TEXT("CurrentHP"), CurHP);
        const bool bHasMaxHP = TryGetInt(P, TEXT("MaxHP"), MaxHP);
        const bool bAlive = !bHasHP ? true : (CurHP > 0);
        if (Cfg.bOnlyLiving && !bAlive) continue;

        // 4) Ÿ�� �ε���
        const int tile = Grid.Y * Cfg.Width + Grid.X;

        // 5) ID / HP / (�ӽ� AP ����)
        const uint16 id = GetOrAssignUnitId(P);
        const int hpForSnapshot = bHasHP ? CurHP : (bHasMaxHP ? MaxHP : 10);
        const int apStub = Cfg.FallbackUnitAP; // S2���� ���� ����

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

    // �� AP Ǯ(���� �� ���� ����)
    Out.teamAP[0] = 0;
    Out.teamAP[1] = 0;
    Out.teamAP[Cfg.SideToAct] = Cfg.TeamAPStart;
    Out.maxAP = std::max(Out.teamAP[0], Out.teamAP[1]);

    // Zobrist ���̺� �غ�
    Out.initZobrist(Cfg.ZobristSeed, (int)Out.units.size());

    if (OutDebugInfo)
    {
        *OutDebugInfo = FString::Printf(TEXT("units=%d WxH=%dx%d side=%d teamAP=[%d,%d]"),
            Count, Cfg.Width, Cfg.Height, Cfg.SideToAct, Out.teamAP[0], Out.teamAP[1]);
    }
    return true;
}