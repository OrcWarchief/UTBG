// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/UTBGHUD.h"
#include "HUD/UserOverlay.h"
#include "PlayerController/UTBGPlayerController.h"
#include "GamePlay/Team/TeamUtils.h"
#include "Board/Board.h"
#include "UTBGComponents/UnitSkillsComponent.h"
#include "Pawn/PawnBase.h"
#include "Data/SkillData.h"  

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameState/UTBGGameState.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTBGHUD, Log, All);

void AUTBGHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetOwningPlayerController();
    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] BeginPlay: PC=%s OverlayClass=%s"),
        *GetNameSafe(PC), *GetNameSafe(OverlayClass));

    if (!PC || !OverlayClass)
    {
        UE_LOG(LogUTBGHUD, Error, TEXT("[HUD] Abort: PC or OverlayClass is null"));
        return;
    }

    // Overlay 생성
    Overlay = CreateWidget<UUserOverlay>(PC, OverlayClass);
    if (Overlay)
    {
        Overlay->AddToViewport(10);
        Overlay->OnEndTurnClicked.AddDynamic(this, &AUTBGHUD::OnEndTurnClicked);
        Overlay->OnSkillBoxClicked.AddDynamic(this, &AUTBGHUD::HandleSkillChosen);

        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] Overlay created: %s (OwningPlayer=%s)"),
            *GetNameSafe(Overlay), *GetNameSafe(PC));
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(Overlay->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);

        PC->SetInputMode(Mode);

        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] SetInputMode=GameAndUI, MouseCursor ON"));
    }
    else
    {
        UE_LOG(LogUTBGHUD, Error, TEXT("[HUD] Failed to create Overlay"));
    }

    // 보드 찾기 & 바인딩
    for (TActorIterator<ABoard> It(GetWorld()); It; ++It)
    {
        CachedBoard = *It;
        break;
    }
    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] CachedBoard = %s"), *GetNameSafe(CachedBoard));

    if (CachedBoard)
    {
        CachedBoard->OnUnitSelected.AddDynamic(this, &AUTBGHUD::HandleUnitSelected);
        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] Bound to Board.OnUnitSelected"));
    }
    else
    {
        UE_LOG(LogUTBGHUD, Error, TEXT("[HUD] No ABoard found to bind OnUnitSelected."));
    }

    // GameState 이벤트 바인딩
    if (AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
    {
        GS->OnTeamAPChanged.AddDynamic(this, &AUTBGHUD::HandleTeamAPChanged);
        GS->OnTurnChanged.AddDynamic(this, &AUTBGHUD::HandleTurnChanged);

        if (Overlay) Overlay->SetIsEnabled(!GS->bResolving);

        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] Bound to GameState events: GS=%s CurrTurn=%d AP=%d MaxAP=%d"),
            *GetNameSafe(GS), (int32)GS->CurrentTurnTeam, GS->CurrentTeamAP, GS->MaxAPPerTurn);
    }
    else
    {
        UE_LOG(LogUTBGHUD, Error, TEXT("[HUD] No GameState found."));
    }

    InitialRefresh();
}

void AUTBGHUD::HandleResolvingChanged(bool bResolving)
{
    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] HandleResolvingChanged: %s"),
        bResolving ? TEXT("LOCK") : TEXT("UNLOCK"));
    if (Overlay)
    {
        Overlay->SetIsEnabled(!bResolving);
    }
}

ETeam AUTBGHUD::GetLocalTeam() const
{
    if (APlayerController* PC = GetOwningPlayerController())
    {
        return UTeamUtils::GetActorTeam(PC);
    }
    return ETeam::ET_NoTeam;
}

void AUTBGHUD::InitialRefresh()
{
    if (!Overlay)
    {
        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] InitialRefresh: Overlay is null"));
        return;
    }
    if (AUTBGGameState* GS = GetWorld()->GetGameState<AUTBGGameState>())
    {
        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] InitialRefresh: LocalTeam=%d Turn=%d AP=%d MaxAP=%d"),
            (int32)GetLocalTeam(), (int32)GS->CurrentTurnTeam, GS->CurrentTeamAP, GS->MaxAPPerTurn);

        Overlay->SetTurnAndAP(
            GetLocalTeam(),
            GS->CurrentTurnTeam,
            GS->CurrentTeamAP,
            GS->MaxAPPerTurn
        );
    }
    else
    {
        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] InitialRefresh: GameState is null"));
    }
}

void AUTBGHUD::HandleTeamAPChanged(ETeam Team, int32 NewAP)
{
    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] HandleTeamAPChanged: Team=%d NewAP=%d"), (int32)Team, NewAP);

    if (!Overlay) { UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD]   -> Overlay is null")); return; }
    if (AUTBGGameState* GS = GetWorld()->GetGameState<AUTBGGameState>())
    {
        Overlay->SetAPForTeam(Team, NewAP, GS->MaxAPPerTurn, GetLocalTeam());
    }
}

void AUTBGHUD::HandleTurnChanged(ETeam NewTurn)
{
    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] HandleTurnChanged: NewTurn=%d"), (int32)NewTurn);

    if (!Overlay) { UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD]   -> Overlay is null")); return; }
    if (AUTBGGameState* GS = GetWorld()->GetGameState<AUTBGGameState>())
    {
        Overlay->SetTurnAndAP(GetLocalTeam(), NewTurn, GS->CurrentTeamAP, GS->MaxAPPerTurn);
    }
}

void AUTBGHUD::HandleMatchEnded(ETeam WinningTeam, FString Reason)
{
    UE_LOG(LogUTBGHUD, Log, TEXT("[HUD] MatchEnded: Winner=%d Reason=%s"), (int32)WinningTeam, *Reason);

    if (Overlay)
    {
        Overlay->SetIsEnabled(false); // 버튼/슬롯 등 인터랙션 비활성화
        // 필요하면 Overlay->SetVisibility(ESlateVisibility::HitTestInvisible); 도 가능
    }
}

void AUTBGHUD::HandleUnitSelected(APawnBase* Unit)
{
    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] HandleUnitSelected: Unit=%s"), *GetNameSafe(Unit));

    if (!Overlay) { UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD]   -> Overlay is null")); return; }

    if (!Unit)
    {
        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD]   -> No unit: clear skill bar"));
        Overlay->ClearSkillBar();
        return;
    }

    if (UUnitSkillsComponent* Comp = Unit->FindComponentByClass<UUnitSkillsComponent>())
    {
        const TArray<USkillData*>& Skills = Comp->GetSkillsArray();
        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD]   -> BuildSkillBar: Skills=%d for Unit=%s"),
            Skills.Num(), *GetNameSafe(Unit));
        Overlay->BuildSkillBar(Unit, Skills);
    }
    else
    {
        UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD]   -> Unit has NO UnitSkillsComponent: build empty bar"));
        Overlay->BuildSkillBar(Unit, TArray<USkillData*>());
    }
}

void AUTBGHUD::HandleSkillChosen(APawnBase* User, FName SkillId)
{
    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] HandleSkillChosen: User=%s SkillId=%s"),
        *GetNameSafe(User), *SkillId.ToString());

    if (!User) { UE_LOG(LogUTBGHUD, Error, TEXT("[HUD]   -> User null")); return; }

    UUnitSkillsComponent* Skills = User->FindComponentByClass<UUnitSkillsComponent>();
    if (!Skills) { UE_LOG(LogUTBGHUD, Error, TEXT("[HUD]   -> User has no UnitSkillsComponent")); return; }

    USkillData* Data = Skills->FindSkillById(SkillId);
    if (!Data) { UE_LOG(LogUTBGHUD, Error, TEXT("[HUD]   -> Skill not found: %s"), *SkillId.ToString()); return; }

    if (AUTBGGameState* GS = GetWorld()->GetGameState<AUTBGGameState>())
    {
        if (!GS->IsActorTurn(User))
        {
            UE_LOG(LogUTBGHUD, Verbose, TEXT("[HUD]   -> BLOCK: Not your turn"));
            return;
        }
        if (GS->bResolving)
        {
            UE_LOG(LogUTBGHUD, Verbose, TEXT("[HUD]   -> BLOCK: Resolving in progress"));
            return;
        }
        if (!GS->HasAPForActor(User, Data->APCost))
        {
            UE_LOG(LogUTBGHUD, Verbose, TEXT("[HUD]   -> BLOCK: Not enough AP"));
            return;
        }
    }

    FText Reason;
    if (!Skills->CanUseByCooldownOnly(Data, Reason))
    {
        UE_LOG(LogUTBGHUD, Verbose, TEXT("[HUD]   -> BLOCK: %s"), *Reason.ToString());
        return;
    }

    // 타깃(임시: Self 우선)
    AActor* TargetActor = nullptr;
    switch (Data->TargetMode)
    {
    case ESkillTargetMode::Self: TargetActor = User; break;
    default:                     TargetActor = User; break; // 추후 타깃팅 UI 붙이면 교체
    }

    // 사거리 빠른 가드
    if (TargetActor)
    {
        FText RangeReason;
        if (!Skills->CanUseByRangeOnly(Data, TargetActor, RangeReason))
        {
            UE_LOG(LogUTBGHUD, Verbose, TEXT("[HUD]   -> BLOCK: %s"), *RangeReason.ToString());
            return;
        }
    }

    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD]   -> StartSkill_NotifyDriven(Data=%s, Target=%s)"),
        *GetNameSafe(Data), *GetNameSafe(TargetActor));

    User->StartSkill_NotifyDriven(Data, TargetActor);
}

void AUTBGHUD::OnEndTurnClicked()
{
    UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD] OnEndTurnClicked"));

    if (APlayerController* PC = GetOwningPlayerController())
    {
        if (AUTBGPlayerController* MyPC = Cast<AUTBGPlayerController>(PC))
        {
            UE_LOG(LogUTBGHUD, Warning, TEXT("[HUD]   -> RPC ServerEndTurn()"));
            MyPC->ServerEndTurn();
        }
        else
        {
            UE_LOG(LogUTBGHUD, Error, TEXT("[HUD]   -> Owning PC cast to AUTBGPlayerController failed"));
        }
    }
    else
    {
        UE_LOG(LogUTBGHUD, Error, TEXT("[HUD]   -> No owning PC"));
    }
}