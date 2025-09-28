// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/SkillTypes.h"
#include "SkillData.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class UTBG_API USkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
    // �ĺ�/ǥ��
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info")
    FName SkillId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Info", meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UTexture2D> Icon;

    // �ڽ�Ʈ/��Ģ
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
    int32 APCost = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
    int32 CooldownTurns = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
    bool bEndsTurn = false;

    // Ÿ����
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
    ESkillTargetMode TargetMode = ESkillTargetMode::Unit;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
    ETeamFilter TeamFilter = ETeamFilter::Enemy;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
    int32 Range = 5; // (����ư/ü����� �Ծ��� ������Ʈ �������� ����)

    // �ִ�/����
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> CastMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    FName CastSection = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0"))
    float HitTimeSec = 0.3f;

    // ȿ�� ����(������/���� ��)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
    TArray<FSkillEffect> Effects;
};
