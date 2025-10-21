// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SkillTypes.h"
#include "UnitSkillsComponent.generated.h"

class USkillData;
class UNiagaraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCooldownsUpdated);

UCLASS(BlueprintType, Blueprintable, ClassGroup = (UTBG), meta = (BlueprintSpawnableComponent))
class UTBG_API UUnitSkillsComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UUnitSkillsComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 각 유닛이 가진 스킬 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
	TArray<TObjectPtr<USkillData>> Skills;


protected:
    // 슬롯별 남은 쿨다운(서버 권위, 복제)
    UPROPERTY(ReplicatedUsing = OnRep_Cooldowns, VisibleInstanceOnly, Category = "Skills")
    TArray<int32> Cooldowns;

    UFUNCTION() 
    void OnRep_Cooldowns();

    virtual void BeginPlay() override;

public:
    UPROPERTY(BlueprintAssignable, Category = "Skills|Events")
    FOnCooldownsUpdated OnCooldownsUpdated;


    UFUNCTION(BlueprintCallable, Category = "Skills")
    void EnsureCooldownSize();

    UFUNCTION(BlueprintCallable, Category = "Skills")
    USkillData* FindSkillById(FName SkillId) const;

    UFUNCTION(BlueprintCallable, Category = "Skills")
    int32 GetCooldownRemaining(const USkillData* Data) const;

    UFUNCTION(BlueprintCallable, Category = "Skills")
    bool IsOnCooldown(const USkillData* Data) const;

    // 턴 시작 훅(게임스테이트에서 호출): CD-1
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void OnTurnStarted();

    // 사용 직후 쿨다운 시작(서버에서 호출)
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void StartCooldown(const USkillData* Data);

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void StartCooldownById(FName SkillId);

    // UI용 빠른 체크(쿨다운만 확인; AP/사거리/팀필터는 실행 단계에서 서버 재검증)
    UFUNCTION(BlueprintCallable, Category = "Skills")
    bool CanUseByCooldownOnly(const USkillData* Data, FText& OutReason) const;

    const TArray<USkillData*>& GetSkillsArray() const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    void GetSkillsCopy(TArray<USkillData*>& OutSkills) const;

    // --------------------- (옵션) Range 전용 헬퍼 ---------------------
    UFUNCTION(BlueprintCallable, Category = "Skills|Rules")
    bool CanUseByRangeOnly(const USkillData* Data, const AActor* Target, FText& OutReason) const;

    UFUNCTION(BlueprintCallable, Category = "Skills|Rules")
    bool CanReachLocation(const USkillData* Data, const FVector& TargetLocation, FText& OutReason) const;

    UFUNCTION(BlueprintPure, Category = "Skills|Rules")
    float GetRangeInUU(const USkillData* Data) const;

    // --------------------- VFX / Projectile API ---------------------
    // 캐스팅 시작/종료, 임팩트(원샷) 재생
    UFUNCTION(BlueprintCallable, Category = "Skills|VFX")
    void PlayCastVfx(const USkillData* Data);

    UFUNCTION(BlueprintCallable, Category = "Skills|VFX")
    void StopCastVfx(const USkillData* Data);

    UFUNCTION(BlueprintCallable, Category = "Skills|VFX")
    void PlayImpactVfx(const USkillData* Data, AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Skills|Internal")
    void ApplyEffectsAndCosts_Server(const USkillData* Data, AActor* Target);

    // 즉발/직사/곡사 통합. 내부에서 거리/속도→이동시간 계산, 근거리 스냅 처리
    UFUNCTION(BlueprintCallable, Category = "Skills|VFX")
    void PlayProjectileOrInstant(const USkillData* Data, AActor* Target);

protected:
    // --------------------- VFX RPC ---------------------
    UFUNCTION(Server, Reliable)
    void Server_PlaySimpleVfx(FName SkillId, ESimpleVfxSlot Slot, AActor* Target);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlaySimpleVfx(FName SkillId, ESimpleVfxSlot Slot, AActor* Source, AActor* Target);

    UFUNCTION(Server, Reliable)
    void Server_StopSimpleVfx(FName SkillId, ESimpleVfxSlot Slot);

    // 루프 누수 방지를 위해 Reliable
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_StopSimpleVfx(FName SkillId, ESimpleVfxSlot Slot);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_StartProjectile(FName SkillId, FVector_NetQuantize Start, FVector_NetQuantize End, float TravelTime);

private:
    // --------------------- 로컬 재생 구현 ---------------------
    void PlaySimpleVfx_Local(const USkillData* Data, ESimpleVfxSlot Slot, AActor* Source, AActor* Target);
    void StopSimpleVfx_Local(const USkillData* Data, ESimpleVfxSlot Slot);
    void StartProjectile_Local(const USkillData* Data, const FVector& Start, const FVector& End, float TravelTime);

    static FName MakeVfxKey(FName SkillId, ESimpleVfxSlot Slot);
    static const FSimpleVfxSpec& GetSpec(const USkillData* Data, ESimpleVfxSlot Slot);

    static int32 ComputeTilesDistance2D(const FVector& A, const FVector& B, EGridDistanceMetric Metric, float TileSizeUU = 100.f);

    // 루프 VFX 관리
    UPROPERTY(Transient)
    TMap<FName, TWeakObjectPtr<UNiagaraComponent>> ActiveLoopVfx;

    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<UNiagaraComponent>, FTimerHandle> ProjectileMoveTimers;

    mutable TArray<USkillData*> SkillsRawCache;

    FORCEINLINE bool HasServerAuthority() const { const AActor* O = GetOwner(); return O && O->HasAuthority(); }
};
