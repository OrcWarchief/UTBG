// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamePlay/Team/Team.h"
#include "EndGameWidget.generated.h"

class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEGExitClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEGRematchClicked);

UCLASS()
class UTBG_API UEndGameWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "EndGame")
    void InitializeWithResult(ETeam Winner, FText InReason, ETeam LocalTeam);

    UFUNCTION(BlueprintCallable, Category = "EndGame")
    void SetButtonsEnabled(bool bEnabled);

    UPROPERTY(BlueprintAssignable, Category = "EndGame|Events")
    FOnEGExitClicked OnExitClicked;

    UPROPERTY(BlueprintAssignable, Category = "EndGame|Events")
    FOnEGRematchClicked OnRematchClicked;
protected:
    virtual void NativeConstruct() override;

    UFUNCTION() void HandleExit();
    UFUNCTION() void HandleRematch();

    UPROPERTY(meta = (BindWidgetOptional)) 
    UTextBlock* TitleText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) 
    UTextBlock* ReasonText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) 
    UButton* ExitButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) 
    UButton* RematchButton = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EndGame|Style")
    FLinearColor VictoryColor = FLinearColor(0.00f, 0.63f, 1.0f);
    UPROPERTY(EditDefaultsOnly, Category = "EndGame|Style")
    FLinearColor DefeatColor = FLinearColor(0.86f, 0.25f, 0.25f);
    UPROPERTY(EditDefaultsOnly, Category = "EndGame|Style")
    FLinearColor DrawColor = FLinearColor(0.70f, 0.70f, 0.70f);
};
