// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/UserOverlay.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "HUD/SkillBoxWidget.h"  

#include "Pawn/PawnBase.h"
#include "Data/SkillData.h" 

DEFINE_LOG_CATEGORY_STATIC(LogOverlay, Log, All);

bool UUserOverlay::Initialize()
{
    const bool bOK = Super::Initialize();
    if (EndTurnButton)
    {
        EndTurnButton->OnClicked.RemoveAll(this);
        EndTurnButton->OnClicked.AddDynamic(this, &UUserOverlay::HandleEndTurnButtonClicked);
    }
    UE_LOG(LogOverlay, Warning, TEXT("[Overlay] Initialize: Skillbarbox=%s SkillBoxWidgetClass=%s"),
        *GetNameSafe(Skillbarbox), *GetNameSafe(SkillBoxWidgetClass));
    return bOK;
}

void UUserOverlay::BuildSkillBar(APawnBase* UserUnit, const TArray<USkillData*>& Skills)
{
    if (!Skillbarbox || !SkillBoxWidgetClass)
    {
        UE_LOG(LogOverlay, Error, TEXT("[Overlay] BuildSkillBar FAIL: Skillbarbox=%p Class=%s"),
            Skillbarbox, *GetNameSafe(SkillBoxWidgetClass));
        return;
    }

    Skillbarbox->ClearChildren();
    UE_LOG(LogOverlay, Warning, TEXT("[Overlay] BuildSkillBar: User=%s Skills=%d"),*GetNameSafe(UserUnit), Skills.Num());
    const bool bHasSkills = Skills.Num() > 0;

    Skillbarbox->SetVisibility(bHasSkills ? ESlateVisibility::SelfHitTestInvisible
        : ESlateVisibility::Collapsed);
    Skillbarbox->SetIsEnabled(true);


    for (USkillData* SkillData : Skills)
    {
        if (!SkillData) continue;

        USkillBoxWidget* Box = CreateWidget<USkillBoxWidget>(GetOwningPlayer(), SkillBoxWidgetClass);
        if (!Box) { UE_LOG(LogOverlay, Error, TEXT("[Overlay]   CreateWidget FAILED")); continue; }

        Box->Setup(UserUnit, SkillData);
        Box->OnSkillClicked.RemoveAll(this);
        Box->OnSkillClicked.AddUniqueDynamic(this, &UUserOverlay::HandleSkillBoxClicked);

        Skillbarbox->AddChildToHorizontalBox(Box);

        UE_LOG(LogOverlay, Warning, TEXT("[Overlay]   Added SkillBox: %s (%s)"),
            *SkillData->SkillId.ToString(), *SkillData->DisplayName.ToString());
    }

    UE_LOG(LogOverlay, Warning, TEXT("[Overlay]   Children after build = %d"),
        Skillbarbox->GetChildrenCount());

    Skillbarbox->ForceLayoutPrepass();
}

void UUserOverlay::ClearSkillBar()
{
    if (!Skillbarbox) return;
    Skillbarbox->ClearChildren();
    Skillbarbox->SetVisibility(ESlateVisibility::Collapsed);
    UE_LOG(LogOverlay, Warning, TEXT("[Overlay] ClearSkillBar]"));
}

void UUserOverlay::HandleEndTurnButtonClicked()
{
    OnEndTurnClicked.Broadcast();
    EndTurnNative.ExecuteIfBound();
}

void UUserOverlay::HandleSkillBoxClicked(APawnBase* User, FName SkillId)
{
    UE_LOG(LogOverlay, Warning, TEXT("[Overlay] OnSkillClicked -> HUD delegate: %s / %s"),
        *GetNameSafe(User), *SkillId.ToString());
    OnSkillBoxClicked.Broadcast(User, SkillId);
}

void UUserOverlay::RebuildPipRow(UHorizontalBox* Row, int32 AP, int32 MaxAP)
{
    if (!Row || !PipWidgetClass) return;
    Row->ClearChildren();

    for (int32 i = 0; i < MaxAP; ++i)
    {
        UUserWidget* Pip = CreateWidget<UUserWidget>(this, PipWidgetClass);
        if (!Pip) continue;

        const bool bFilled = (i < AP);
        Pip->SetRenderOpacity(bFilled ? 1.f : 0.25f);
        Row->AddChildToHorizontalBox(Pip);
    }
}

void UUserOverlay::SetAPForTeam(ETeam TeamToUpdate, int32 AP, int32 MaxAP, ETeam LocalTeam)
{
    UHorizontalBox* Target = (TeamToUpdate == LocalTeam) ? MyAPBox : EnemyAPBox;
    RebuildPipRow(Target, AP, MaxAP);
}

void UUserOverlay::SetTurnAndAP(ETeam LocalTeam, ETeam CurrentTurn, int32 CurrentTeamAP, int32 MaxAP)
{
    const bool bMyTurn = (CurrentTurn == LocalTeam);

    if (TurnText)
    {
        TurnText->SetText(FText::FromString(bMyTurn ? TEXT("END TURN") : TEXT("END TURN")));
    }
    if (EndTurnButton)
    {
        EndTurnButton->SetIsEnabled(bMyTurn);
        EndTurnButton->SetVisibility(ESlateVisibility::Visible);
        EndTurnButton->SetRenderOpacity(bMyTurn ? 1.f : 0.6f);
    }

    if (bMyTurn)
    {
        RebuildPipRow(MyAPBox, CurrentTeamAP, MaxAP);
        RebuildPipRow(EnemyAPBox, MaxAP, MaxAP);
    }
    else
    {
        RebuildPipRow(MyAPBox, MaxAP, MaxAP);
        RebuildPipRow(EnemyAPBox, CurrentTeamAP, MaxAP);
    }
}