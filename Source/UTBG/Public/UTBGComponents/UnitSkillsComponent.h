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

	// �� ������ ���� ��ų ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
	TArray<TObjectPtr<USkillData>> Skills;


protected:
    // ���Ժ� ���� ��ٿ�(���� ����, ����)
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

    // �� ���� ��(���ӽ�����Ʈ���� ȣ��): CD-1
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void OnTurnStarted();

    // ��� ���� ��ٿ� ����(�������� ȣ��)
    UFUNCTION(BlueprintCallable, Category = "Skills")
    void StartCooldown(const USkillData* Data);

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void StartCooldownById(FName SkillId);

    // UI�� ���� üũ(��ٿ Ȯ��; AP/��Ÿ�/�����ʹ� ���� �ܰ迡�� ���� �����)
    UFUNCTION(BlueprintCallable, Category = "Skills")
    bool CanUseByCooldownOnly(const USkillData* Data, FText& OutReason) const;

    const TArray<USkillData*>& GetSkillsArray() const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    void GetSkillsCopy(TArray<USkillData*>& OutSkills) const;

    // --------------------- (�ɼ�) Range ���� ���� ---------------------
    UFUNCTION(BlueprintCallable, Category = "Skills|Rules")
    bool CanUseByRangeOnly(const USkillData* Data, const AActor* Target, FText& OutReason) const;

    UFUNCTION(BlueprintCallable, Category = "Skills|Rules")
    bool CanReachLocation(const USkillData* Data, const FVector& TargetLocation, FText& OutReason) const;

    UFUNCTION(BlueprintPure, Category = "Skills|Rules")
    float GetRangeInUU(const USkillData* Data) const;

    // --------------------- VFX / Projectile API ---------------------
    // ĳ���� ����/����, ����Ʈ(����) ���
    UFUNCTION(BlueprintCallable, Category = "Skills|VFX")
    void PlayCastVfx(const USkillData* Data);

    UFUNCTION(BlueprintCallable, Category = "Skills|VFX")
    void StopCastVfx(const USkillData* Data);

    UFUNCTION(BlueprintCallable, Category = "Skills|VFX")
    void PlayImpactVfx(const USkillData* Data, AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Skills|Internal")
    void ApplyEffectsAndCosts_Server(const USkillData* Data, AActor* Target);

    // ���/����/��� ����. ���ο��� �Ÿ�/�ӵ����̵��ð� ���, �ٰŸ� ���� ó��
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

    // ���� ���� ������ ���� Reliable
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_StopSimpleVfx(FName SkillId, ESimpleVfxSlot Slot);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_StartProjectile(FName SkillId, FVector_NetQuantize Start, FVector_NetQuantize End, float TravelTime);

private:
    // --------------------- ���� ��� ���� ---------------------
    void PlaySimpleVfx_Local(const USkillData* Data, ESimpleVfxSlot Slot, AActor* Source, AActor* Target);
    void StopSimpleVfx_Local(const USkillData* Data, ESimpleVfxSlot Slot);
    void StartProjectile_Local(const USkillData* Data, const FVector& Start, const FVector& End, float TravelTime);

    static FName MakeVfxKey(FName SkillId, ESimpleVfxSlot Slot);
    static const FSimpleVfxSpec& GetSpec(const USkillData* Data, ESimpleVfxSlot Slot);

    static int32 ComputeTilesDistance2D(const FVector& A, const FVector& B, EGridDistanceMetric Metric, float TileSizeUU = 100.f);

    // ���� VFX ����
    UPROPERTY(Transient)
    TMap<FName, TWeakObjectPtr<UNiagaraComponent>> ActiveLoopVfx;

    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<UNiagaraComponent>, FTimerHandle> ProjectileMoveTimers;

    mutable TArray<USkillData*> SkillsRawCache;

    FORCEINLINE bool HasServerAuthority() const { const AActor* O = GetOwner(); return O && O->HasAuthority(); }
};
