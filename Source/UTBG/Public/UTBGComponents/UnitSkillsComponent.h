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

private:
    mutable TArray<USkillData*> SkillsRawCache;
};
