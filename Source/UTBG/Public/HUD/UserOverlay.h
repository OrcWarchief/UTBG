// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamePlay/Team/Team.h"
#include "UserOverlay.generated.h"

class UHorizontalBox;
class UTextBlock;
class UButton;
class USkillBoxWidget;
class APawnBase;
class UUnitSkillsComponent;
class USkillData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndTurnClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillBoxClicked, APawnBase*, User, FName, SkillId);


/**
 * 
 */
UCLASS()
class UTBG_API UUserOverlay : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual bool Initialize() override;

    // === BindWidget�� === (�����̳ʿ��� �̸� ���߱�)
    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* MyAPBox = nullptr;
    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* EnemyAPBox = nullptr;
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TurnText = nullptr;
    UPROPERTY(meta = (BindWidget))
    UButton* EndTurnButton = nullptr;

    // Pip ������
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PipWidgetClass;

    // ��ų �ڽ�
    UPROPERTY(meta = (BindWidget)) 
    UHorizontalBox* Skillbarbox = nullptr;
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USkillBoxWidget> SkillBoxWidgetClass;
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillBoxClicked OnSkillBoxClicked;

    // ��ư Ŭ���� HUD�� �˸�
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEndTurnClicked OnEndTurnClicked;

    // HUD���� ȣ���ϴ� API
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetTurnAndAP(ETeam LocalTeam, ETeam CurrentTurn, int32 CurrentTeamAP, int32 MaxAP);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetAPForTeam(ETeam TeamToUpdate, int32 AP, int32 MaxAP, ETeam LocalTeam);

    void BindEndTurn(FSimpleDelegate InDelegate) { EndTurnNative = MoveTemp(InDelegate); }

    UFUNCTION(BlueprintCallable, Category = "UI")
    void BuildSkillBar(APawnBase* UserUnit, const TArray<USkillData*>& Skills);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ClearSkillBar();

private:
    UFUNCTION() 
    void HandleEndTurnButtonClicked();
    UFUNCTION()
    void HandleSkillBoxClicked(APawnBase* User, FName SkillId);
    void RebuildPipRow(class UHorizontalBox* Row, int32 AP, int32 MaxAP);

    FSimpleDelegate EndTurnNative;
};
