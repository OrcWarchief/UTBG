// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitSkillsComponent.generated.h"

class USkillData;

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

private:
    mutable TArray<USkillData*> SkillsRawCache;
};
