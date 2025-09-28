// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Types/SlateEnums.h" 
#include "SkillBoxWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class USkillData;
class APawnBase;

DECLARE_LOG_CATEGORY_EXTERN(LogSkillBox, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillClicked, APawnBase*, User, FName, SkillId);
/**
 * 
 */
UCLASS()
class UTBG_API USkillBoxWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // 스폰 시 주입
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = "true"))
    APawnBase* UserUnit = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = "true"))
    USkillData* SkillData = nullptr;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillClicked OnSkillClicked;

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void Setup(APawnBase* InUser, USkillData* InData);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void Refresh();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void SetUsable(bool bInUsable);

    // TurnsLeft < 0 이면 쿨다운 비표시
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void SetCooldownTurns(int32 TurnsLeft);

protected:
    UPROPERTY(meta = (BindWidget))
    UButton* ButtonSkill = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* ImageSkill = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* TextSkill = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* TextCooldown = nullptr;

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;


private:
    UFUNCTION()
    void OnButtonClicked();
    
    bool  bUsable = true;
    int32 CachedCooldownTurns = 0;

    UFUNCTION() void OnPressed_Debug();
    UFUNCTION() void OnReleased_Debug();
    bool bFiredThisPress = false;
};
