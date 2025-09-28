// Fill out your copyright notice in the Description page of Project Settings.


#include "UTBGComponents/UnitSkillsComponent.h"
#include "Data/SkillData.h"
#include "Net/UnrealNetwork.h"

UUnitSkillsComponent::UUnitSkillsComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}

void UUnitSkillsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UUnitSkillsComponent, Cooldowns);
}

void UUnitSkillsComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureCooldownSize();
}

void UUnitSkillsComponent::OnRep_Cooldowns()
{
    OnCooldownsUpdated.Broadcast();
}

void UUnitSkillsComponent::EnsureCooldownSize()
{
    if (Cooldowns.Num() != Skills.Num())
    {
        const int32 OldNum = Cooldowns.Num();
        Cooldowns.SetNum(Skills.Num());
        for (int32 i = OldNum; i < Cooldowns.Num(); ++i)
        {
            Cooldowns[i] = 0;
        }
    }
}

USkillData* UUnitSkillsComponent::FindSkillById(FName SkillId) const
{
    for (USkillData* S : Skills)
    {
        if (S && S->SkillId == SkillId) return S;
    }
    return nullptr;
}

int32 UUnitSkillsComponent::GetCooldownRemaining(const USkillData* Data) const
{
    if (!Data) return 0;
    const int32 Index = Skills.IndexOfByKey(Data);
    if (Index == INDEX_NONE) return 0;
    if (!Cooldowns.IsValidIndex(Index)) return 0;
    return FMath::Max(0, Cooldowns[Index]);
}

bool UUnitSkillsComponent::IsOnCooldown(const USkillData* Data) const
{
    return GetCooldownRemaining(Data) > 0;
}

void UUnitSkillsComponent::OnTurnStarted()
{
    EnsureCooldownSize();
    for (int32& Cd : Cooldowns)
    {
        Cd = FMath::Max(0, Cd - 1);
    }

    OnCooldownsUpdated.Broadcast();
}

void UUnitSkillsComponent::StartCooldown(const USkillData* Data)
{
    if (!Data) return;
    EnsureCooldownSize();
    const int32 Index = Skills.IndexOfByKey(Data);
    if (Index != INDEX_NONE && Cooldowns.IsValidIndex(Index))
    {
        Cooldowns[Index] = Data->CooldownTurns;
        OnCooldownsUpdated.Broadcast();
    }
}

void UUnitSkillsComponent::StartCooldownById(FName SkillId)
{
    if (USkillData* Data = FindSkillById(SkillId))
    {
        StartCooldown(Data);
    }
}

bool UUnitSkillsComponent::CanUseByCooldownOnly(const USkillData* Data, FText& OutReason) const
{
    if (!Data)
    {
        OutReason = FText::FromString(TEXT("Invalid skill"));
        return false;
    }
    const int32 Remain = GetCooldownRemaining(Data);
    if (Remain > 0)
    {
        OutReason = FText::FromString(FString::Printf(TEXT("Cooldown %d"), Remain));
        return false;
    }
    return true;
}

const TArray<USkillData*>& UUnitSkillsComponent::GetSkillsArray() const
{
    // C++ 용: 내부 캐시를 갱신해 참조 반환
    SkillsRawCache.Reset();
    SkillsRawCache.Reserve(Skills.Num());
    for (USkillData* S : Skills)
    {
        SkillsRawCache.Add(S);
    }
    return SkillsRawCache;
}

void UUnitSkillsComponent::GetSkillsCopy(TArray<USkillData*>& OutSkills) const
{
    OutSkills.Reset();
    OutSkills.Reserve(Skills.Num());
    for (USkillData* S : Skills)
    {
        OutSkills.Add(S);
    }
}