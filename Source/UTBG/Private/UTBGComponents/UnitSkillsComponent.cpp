// Fill out your copyright notice in the Description page of Project Settings.


#include "UTBGComponents/UnitSkillsComponent.h"
#include "Data/SkillData.h"
#include "Pawn/PawnBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameState/UTBGGameState.h"
#include "GamePlay/Team/TeamUtils.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTBGSkills, Log, All);

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
    if (!HasServerAuthority()) return;
    EnsureCooldownSize();
    for (int32& Cd : Cooldowns)
    {
        Cd = FMath::Max(0, Cd - 1);
    }
    UE_LOG(LogUTBGSkills, Verbose, TEXT("[CD] OnTurnStarted: %s"), *GetNameSafe(GetOwner()));
    OnCooldownsUpdated.Broadcast();
}

void UUnitSkillsComponent::StartCooldown(const USkillData* Data)
{
    if (!HasServerAuthority() || !Data) return;
    EnsureCooldownSize();
    const int32 Index = Skills.IndexOfByKey(Data);
    if (Index != INDEX_NONE && Cooldowns.IsValidIndex(Index))
    {
        Cooldowns[Index] = Data->CooldownTurns;
        UE_LOG(LogUTBGSkills, Verbose, TEXT("[CD] Start %s -> %d turns"), *Data->SkillId.ToString(), Data->CooldownTurns);
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

int32 UUnitSkillsComponent::ComputeTilesDistance2D(const FVector& A, const FVector& B, EGridDistanceMetric Metric, float TileSizeUU)
{
    const float dx = FMath::Abs(A.X - B.X) / FMath::Max(1.f, TileSizeUU);
    const float dy = FMath::Abs(A.Y - B.Y) / FMath::Max(1.f, TileSizeUU);
    const int32 ix = FMath::RoundToInt(dx);
    const int32 iy = FMath::RoundToInt(dy);
    switch (Metric)
    {
    default:
    case EGridDistanceMetric::Chebyshev: return FMath::Max(ix, iy);
    case EGridDistanceMetric::Manhattan: return ix + iy;
    }
}

bool UUnitSkillsComponent::CanUseByRangeOnly(const USkillData* Data, const AActor* Target, FText& OutReason) const
{
    if (!Data || !Target || !GetOwner())
    {
        OutReason = FText::FromString(TEXT("Invalid owner/target"));
        return false;
    }
    const FVector A = GetOwner()->GetActorLocation();
    const FVector B = Target->GetActorLocation();
    const int32 tiles = ComputeTilesDistance2D(A, B, Data->RangeMetric, 100.f);
    const bool ok = tiles <= Data->Range;
    if (!ok)
    {
        OutReason = FText::FromString(FString::Printf(TEXT("Out of range (%d/%d)"), tiles, Data->Range));
    }
    return ok;
}

bool UUnitSkillsComponent::CanReachLocation(const USkillData* Data, const FVector& TargetLocation, FText& OutReason) const
{
    if (!Data || !GetOwner())
    {
        OutReason = FText::FromString(TEXT("Invalid owner/skill"));
        return false;
    }
    const FVector A = GetOwner()->GetActorLocation();
    const int32 tiles = ComputeTilesDistance2D(A, TargetLocation, Data->RangeMetric, 100.f);
    const bool ok = tiles <= Data->Range;
    if (!ok)
    {
        OutReason = FText::FromString(FString::Printf(TEXT("Out of range (%d/%d)"), tiles, Data->Range));
    }
    return ok;
}

float UUnitSkillsComponent::GetRangeInUU(const USkillData* Data) const
{
    return Data ? (float)Data->Range * 100.f : 0.f;
}

// ------------------------- VFX API -------------------------

void UUnitSkillsComponent::PlayCastVfx(const USkillData* Data)
{
    if (!Data || !GetOwner() || !HasServerAuthority()) return;
    UE_LOG(LogUTBGSkills, Verbose, TEXT("[VFX] Cast START %s on %s"), *Data->SkillId.ToString(), *GetNameSafe(GetOwner()));
    Multicast_PlaySimpleVfx(Data->SkillId, ESimpleVfxSlot::Cast, GetOwner(), nullptr);
}

void UUnitSkillsComponent::StopCastVfx(const USkillData* Data)
{
    if (!Data || !GetOwner() || !HasServerAuthority()) return;
    UE_LOG(LogUTBGSkills, Verbose, TEXT("[VFX] Cast STOP %s on %s"), *Data->SkillId.ToString(), *GetNameSafe(GetOwner()));
    Multicast_StopSimpleVfx(Data->SkillId, ESimpleVfxSlot::Cast);
}

void UUnitSkillsComponent::PlayImpactVfx(const USkillData* Data, AActor* Target)
{
    if (!Data || !GetOwner() || !HasServerAuthority()) return;
    UE_LOG(LogUTBGSkills, Warning, TEXT("[VFX] Impact %s: Source=%s Target=%s"),
        *Data->SkillId.ToString(), *GetNameSafe(GetOwner()), *GetNameSafe(Target));
    Multicast_PlaySimpleVfx(Data->SkillId, ESimpleVfxSlot::Impact, GetOwner(), Target);
}

void UUnitSkillsComponent::PlayProjectileOrInstant(const USkillData* Data, AActor* Target)
{
    if (!Data || !Target || !GetOwner() || !HasServerAuthority()) return;

    const auto& P = Data->Projectile;
    UE_LOG(LogUTBGSkills, Warning, TEXT("[Skill] ProjectileOrInstant %s use=%d motion=%d"),
        *Data->SkillId.ToString(), P.bUseProjectile ? 1 : 0, (int32)P.Motion);

    // 즉발
    if (!P.bUseProjectile || P.Motion == EProjectileMotion::Instant)
    {
        ApplyEffectsAndCosts_Server(Data, Target);
        PlayImpactVfx(Data, Target);
        return;
    }

    // 시작/끝 위치
    FVector Start = GetOwner()->GetActorLocation();
    if (P.bAttachToSource)
    {
        if (const USkeletalMeshComponent* Skel = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (P.SourceSocket != NAME_None && Skel->DoesSocketExist(P.SourceSocket))
            {
                const FTransform T = Skel->GetSocketTransform(P.SourceSocket, RTS_World);
                Start = T.GetLocation();
            }
        }
    }
    const FVector End = Target->GetActorLocation();

    // 근거리 스냅 → 즉발 처리
    const float Dist = FVector::Distance(Start, End);
    if (Dist <= P.ShortAsInstantDistance)
    {
        ApplyEffectsAndCosts_Server(Data, Target);
        PlayImpactVfx(Data, Target);
        return;
    }

    // 이동 시간
    const float Speed = FMath::Max(1.f, P.Speed);
    float TravelTime = Dist / Speed;
    TravelTime = FMath::Clamp(TravelTime, P.MinTravelTime, P.MaxTravelTime);

    // 모든 클라에 투사체 시작 전파
    Multicast_StartProjectile(Data->SkillId, Start, End, TravelTime);

    // 도착 시 효과+임팩트
    FTimerHandle H;
    GetWorld()->GetTimerManager().SetTimer(
        H,
        FTimerDelegate::CreateWeakLambda(this, [this, Data, Target]()
            {
                ApplyEffectsAndCosts_Server(Data, Target);
                PlayImpactVfx(Data, Target);
            }),
        TravelTime,
        false
    );
}

void UUnitSkillsComponent::ApplyEffectsAndCosts_Server(const USkillData* Data, AActor* Target)
{
    if (!HasServerAuthority() || !Data) return;

    APawnBase* Caster = Cast<APawnBase>(GetOwner());
    if (!Caster) return;

    UE_LOG(LogUTBGSkills, Warning, TEXT("[Skill] ApplyEffects %s: Caster=%s Target=%s"),
        *Data->SkillId.ToString(), *GetNameSafe(Caster), *GetNameSafe(Target));

    // 1) 효과 적용(예시)
    for (const FSkillEffect& E : Data->Effects)
    {
        if (E.bDealsDamage && Target)
        {
            UGameplayStatics::ApplyDamage(Target, E.DamageBase, Caster->GetController(), Caster, UDamageType::StaticClass());
        }
        else
        {
            // TODO: 프로젝트 룰에 맞게 버프/실드/스턴/넉백 등 구현
            // 예시: if (APawnBase* PawnCaster = Caster) { PawnCaster->AddShield(5.f); }
        }
    }

    // 2) AP 차감 + (필요 시) 턴 종료 + Resolving 해제
    if (AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
    {
        const bool bSpent = GS->TrySpendAPForActor(Caster, Data->APCost);
        UE_LOG(LogUTBGSkills, Warning, TEXT("[AP] Spend cost=%d -> spent=%d AP=%d"),
            Data->APCost, bSpent ? 1 : 0, GS->GetCurrentAP());

        if (Data->bEndsTurn)
        {
            GS->EndTurn(); // AP 0으로 인한 자동 종료와 중복되어도 안전
        }

        GS->SetResolving(false);
    }

    // 3) 쿨다운 시작
    StartCooldown(Data);
}

// ------------------------- VFX RPC -------------------------
void UUnitSkillsComponent::Server_PlaySimpleVfx_Implementation(FName SkillId, ESimpleVfxSlot Slot, AActor* Target)
{
    Multicast_PlaySimpleVfx(SkillId, Slot, GetOwner(), Target);
}

void UUnitSkillsComponent::Multicast_PlaySimpleVfx_Implementation(FName SkillId, ESimpleVfxSlot Slot, AActor* Source, AActor* Target)
{
    if (USkillData* Data = FindSkillById(SkillId))
    {
        PlaySimpleVfx_Local(Data, Slot, Source, Target);
    }
}

void UUnitSkillsComponent::Server_StopSimpleVfx_Implementation(FName SkillId, ESimpleVfxSlot Slot)
{
    Multicast_StopSimpleVfx(SkillId, Slot);
}

void UUnitSkillsComponent::Multicast_StopSimpleVfx_Implementation(FName SkillId, ESimpleVfxSlot Slot)
{
    if (USkillData* Data = FindSkillById(SkillId))
    {
        StopSimpleVfx_Local(Data, Slot);
    }
}

void UUnitSkillsComponent::Multicast_StartProjectile_Implementation(FName SkillId, FVector_NetQuantize Start, FVector_NetQuantize End, float TravelTime)
{
    if (USkillData* Data = FindSkillById(SkillId))
    {
        StartProjectile_Local(Data, Start, End, TravelTime);
    }
}

// ------------------------- 로컬 재생 구현 -------------------------
static FString SlotToString(ESimpleVfxSlot S)
{
    switch (S)
    {
    case ESimpleVfxSlot::Cast:   return TEXT("Cast");
    case ESimpleVfxSlot::Impact: return TEXT("Impact");
    default:                     return TEXT("Unknown");
    }
}

FName UUnitSkillsComponent::MakeVfxKey(FName SkillId, ESimpleVfxSlot Slot)
{
    return FName(*(SkillId.ToString() + TEXT(".") + SlotToString(Slot)));
}

const FSimpleVfxSpec& UUnitSkillsComponent::GetSpec(const USkillData* Data, ESimpleVfxSlot Slot)
{
    static const FSimpleVfxSpec Empty;
    if (!Data) return Empty;
    switch (Slot)
    {
    case ESimpleVfxSlot::Cast:   return Data->CastVfx;
    case ESimpleVfxSlot::Impact: return Data->ImpactVfx;
    default:                     return Empty;
    }
}

void UUnitSkillsComponent::PlaySimpleVfx_Local(const USkillData* Data, ESimpleVfxSlot Slot, AActor* Source, AActor* Target)
{
    if (!Data) return;
    if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer) return;

    const FSimpleVfxSpec& Spec = GetSpec(Data, Slot);
    if (!Spec.Niagara) return;

    AActor* AttachActor = Spec.bAttach ? (Slot == ESimpleVfxSlot::Impact ? Target : Source) : nullptr;
    FVector SpawnLoc = AttachActor ? AttachActor->GetActorLocation() : (Slot == ESimpleVfxSlot::Impact ? (Target ? Target->GetActorLocation() : FVector::ZeroVector) : (Source ? Source->GetActorLocation() : FVector::ZeroVector));
    FRotator SpawnRot = (Slot == ESimpleVfxSlot::Impact && Source && Target) ? (Target->GetActorLocation() - Source->GetActorLocation()).Rotation() : FRotator::ZeroRotator;

    UNiagaraComponent* NC = nullptr;

    if (AttachActor && Spec.bAttach)
    {
        if (USkeletalMeshComponent* Skel = AttachActor->FindComponentByClass<USkeletalMeshComponent>())
        {
            const FName Sock = Spec.SocketName;
            const bool bHasSock = (Sock != NAME_None) && Skel->DoesSocketExist(Sock);
            NC = UNiagaraFunctionLibrary::SpawnSystemAttached(
                Spec.Niagara,
                bHasSock ? Skel : AttachActor->GetRootComponent(),
                bHasSock ? Sock : NAME_None,
                Spec.LocationOffset,
                Spec.RotationOffset,
                EAttachLocation::KeepRelativeOffset,
                /*bAutoDestroy*/ !Spec.bLoop,
                /*bAutoActivate*/ true);
        }
    }

    if (!NC) // 월드 스폰
    {
        NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            Spec.Niagara,
            SpawnLoc + Spec.LocationOffset,
            (SpawnRot + Spec.RotationOffset).Clamp());
        if (NC && Spec.bLoop == false)
        {
            NC->SetAutoDestroy(true);
        }
    }

    // 루프 관리
    if (NC && Spec.bLoop)
    {
        const FName Key = MakeVfxKey(Data->SkillId, Slot);
        // 기존 루프가 있으면 교체
        if (TWeakObjectPtr<UNiagaraComponent>* Found = ActiveLoopVfx.Find(Key))
        {
            if (Found->IsValid())
            {
                Found->Get()->DestroyComponent();
            }
        }
        ActiveLoopVfx.FindOrAdd(Key) = NC;
    }

    // 사운드
    if (Spec.Sound)
    {
        if (AttachActor && Spec.bAttach)
        {
            UGameplayStatics::SpawnSoundAttached(Spec.Sound, AttachActor->GetRootComponent(), Spec.SocketName,
                Spec.LocationOffset, Spec.RotationOffset, EAttachLocation::KeepRelativeOffset, true);
        }
        else
        {
            UGameplayStatics::PlaySoundAtLocation(this, Spec.Sound, SpawnLoc);
        }
    }
}

void UUnitSkillsComponent::StopSimpleVfx_Local(const USkillData* Data, ESimpleVfxSlot Slot)
{
    if (!Data) return;

    const FSimpleVfxSpec& Spec = GetSpec(Data, Slot);
    const FName Key = MakeVfxKey(Data->SkillId, Slot);

    if (TWeakObjectPtr<UNiagaraComponent>* Found = ActiveLoopVfx.Find(Key))
    {
        if (Found->IsValid())
        {
            if (Spec.LoopStopDelay > 0.f)
            {
                UNiagaraComponent* Comp = Found->Get();
                FTimerHandle H;
                GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [Comp]()
                    {
                        if (IsValid(Comp)) Comp->DestroyComponent();
                    }), Spec.LoopStopDelay, false);
            }
            else
            {
                Found->Get()->DestroyComponent();
            }
        }
        ActiveLoopVfx.Remove(Key);
    }
}

void UUnitSkillsComponent::StartProjectile_Local(const USkillData* Data, const FVector& Start, const FVector& End, float TravelTime)
{
    if (!Data || !Data->Projectile.TravelNiagara) return;
    if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer) return;
   
    const FVector Dir = (End - Start).GetSafeNormal();
    UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this,
        Data->Projectile.TravelNiagara,
        Start,
        Dir.Rotation());

    if (!NC) return;
    NC->SetAutoDestroy(true);

    if (TravelTime <= KINDA_SMALL_NUMBER)
    {
        NC->SetWorldLocation(End);
        return;
    }

    // 간단한 타이머 기반 래핑(0.016s)
    const double StartTime = GetWorld()->GetTimeSeconds();
    const TWeakObjectPtr<UNiagaraComponent> Key = NC;
    FTimerHandle& MoveHandle = ProjectileMoveTimers.FindOrAdd(Key);

    FTimerDelegate D;
    D.BindWeakLambda(this, [this, Key, Start, End, StartTime, TravelTime]()
        {
            UNiagaraComponent* Comp = Key.Get();
            if (!IsValid(Comp))
            {
                if (FTimerHandle* H = ProjectileMoveTimers.Find(Key))
                {
                    GetWorld()->GetTimerManager().ClearTimer(*H);
                    ProjectileMoveTimers.Remove(Key);
                }
                return;
            }

            const double Now = GetWorld()->GetTimeSeconds();
            const float Alpha = FMath::Clamp(float((Now - StartTime) / TravelTime), 0.f, 1.f);
            const FVector Pos = FMath::Lerp(Start, End, Alpha);
            Comp->SetWorldLocation(Pos);
            Comp->SetWorldRotation((End - Start).GetSafeNormal().Rotation());

            if (Alpha >= 1.f - KINDA_SMALL_NUMBER)
            {
                if (FTimerHandle* H = ProjectileMoveTimers.Find(Key))
                {
                    GetWorld()->GetTimerManager().ClearTimer(*H);
                    ProjectileMoveTimers.Remove(Key);
                }
                Comp->DestroyComponent();
            }
        });

    GetWorld()->GetTimerManager().SetTimer(MoveHandle, D, 0.016f, true);
}