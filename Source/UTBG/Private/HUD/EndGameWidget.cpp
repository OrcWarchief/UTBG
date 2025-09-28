// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/EndGameWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UEndGameWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ExitButton)
        ExitButton->OnClicked.AddDynamic(this, &UEndGameWidget::HandleExit);
    if (RematchButton)
        RematchButton->OnClicked.AddDynamic(this, &UEndGameWidget::HandleRematch);
}

void UEndGameWidget::InitializeWithResult(ETeam Winner, FText InReason, ETeam LocalTeam)
{
    FText Title;
    FLinearColor Color;

    if (Winner == ETeam::ET_NoTeam) { Title = FText::FromString(TEXT("DRAW")); Color = DrawColor; }
    else if (Winner == LocalTeam) { Title = FText::FromString(TEXT("VICTORY"));   Color = VictoryColor; }
    else { Title = FText::FromString(TEXT("LOSE"));   Color = DefeatColor; }

    if (TitleText)
    {
        TitleText->SetText(Title);
        TitleText->SetColorAndOpacity(FSlateColor(Color));
    }

    if (ReasonText)
    {
        ReasonText->SetText(InReason);
    }
}

void UEndGameWidget::SetButtonsEnabled(bool bEnabled)
{
    if (ExitButton)    ExitButton->SetIsEnabled(bEnabled);
    if (RematchButton) RematchButton->SetIsEnabled(bEnabled);
}

void UEndGameWidget::HandleExit()
{
    OnExitClicked.Broadcast();
}

void UEndGameWidget::HandleRematch()
{
    OnRematchClicked.Broadcast();
}