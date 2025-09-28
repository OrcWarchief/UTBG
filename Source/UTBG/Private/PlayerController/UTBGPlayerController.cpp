// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/UTBGPlayerController.h"
#include "PlayerState/UTBGPlayerState.h"
#include "Tile/TileActor.h"
#include "Board/Board.h"
#include "Pawn/PawnBase.h"
#include "Data/SkillData.h"
#include "UTBGComponents/UnitSkillsComponent.h"
#include "GameState/UTBGGameState.h"
#include "GamePlay/Team/TeamUtils.h"
#include "HUD/EndGameWidget.h"

#include "Components/InputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "Logging/LogMacros.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h" 

// 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogUTBGPC, Log, All);

// 작은 헬퍼들
static FString Obj(const UObject* O) { return FString::Printf(TEXT("%s[%p]"), *GetNameSafe(O), O); }
static FString Pt(const FIntPoint& P) { return FString::Printf(TEXT("(%d,%d)"), P.X, P.Y); }

static const TCHAR* TeamToStr(ETeam T)
{
	switch (T)
	{
	case ETeam::ET_NoTeam:   return TEXT("NoTeam");
	case ETeam::ET_RedTeam:  return TEXT("Red");
	case ETeam::ET_BlueTeam: return TEXT("Blue");
	default:                 return TEXT("Unknown");
	}
}

static const TCHAR* NetModeToStr(const UWorld* W)
{
	if (!W) return TEXT("NoWorld");
	switch (W->GetNetMode())
	{
	case NM_Standalone:      return TEXT("Standalone");
	case NM_DedicatedServer: return TEXT("DedicatedServer");
	case NM_ListenServer:    return TEXT("ListenServer");
	case NM_Client:          return TEXT("Client");
	default:                 return TEXT("Unknown");
	}
}

ETeam AUTBGPlayerController::GetTeam() const
{
	if (const AUTBGPlayerState* PS = GetPlayerState<AUTBGPlayerState>())
	{
		const ETeam T = PS->GetTeam();
		UE_LOG(LogUTBGPC, Verbose, TEXT("[GetTeam] PC=%s -> %s"), *Obj(this), TeamToStr(T));
		return T;
	}
	UE_LOG(LogUTBGPC, Verbose, TEXT("[GetTeam] PC=%s -> NoTeam(PS null)"), *Obj(this));
	return ETeam::ET_NoTeam;
}

void AUTBGPlayerController::ServerSetTeam_Implementation(ETeam NewTeam)
{
	UE_LOG(LogUTBGPC, Warning, TEXT("[RPC][ServerSetTeam] PC=%s NewTeam=%s HasAuth=%d"),
		*Obj(this), TeamToStr(NewTeam), HasAuthority());

	if (AUTBGPlayerState* PS = GetPlayerState<AUTBGPlayerState>())
	{
		PS->SetTeam(NewTeam); // 서버 권위
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> PS=%s Team set to %s"), *Obj(PS), TeamToStr(PS->GetTeam()));
	}
	else
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> FAIL: PlayerState is null"));
	}
}

void AUTBGPlayerController::ServerTryAttack_Implementation(ABoard* InBoard, APawnBase* Attacker, APawnBase* Target)
{
	if (const AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		if (GS->IsGameOver()) { UE_LOG(LogUTBGPC, Verbose, TEXT("[RPC][Attack] BLOCK: GameOver")); return; }
	}

	UE_LOG(LogUTBGPC, Warning, TEXT("[RPC][Server_TryAttack] PC=%s Board=%s Attacker=%s Target=%s HasAuth=%d"),
		*Obj(this), *Obj(InBoard), *Obj(Attacker), *Obj(Target), HasAuthority());

	if (!InBoard || !Attacker || !Target)
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> FAIL: null param (Board/Attacker/Target)"));
		return;
	}

	// 컨텍스트(턴/AP/팀) 출력
	if (const AUTBGGameState* GS = GetWorld()->GetGameState<AUTBGGameState>())
	{
		const ETeam TurnTeam = GS->CurrentTurnTeam;
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> GS: Turn=%s AP=%d | AttackerTeam=%s TargetTeam=%s"),
			TeamToStr(TurnTeam), GS->GetCurrentAP(),
			TeamToStr(UTeamUtils::GetActorTeam(Attacker)),
			TeamToStr(UTeamUtils::GetActorTeam(Target)));
	}

	const bool bOk = InBoard->TryAttackUnit(Attacker, Target);
	UE_LOG(LogUTBGPC, Warning, TEXT("  -> TryAttackUnit = %s"), bOk ? TEXT("TRUE") : TEXT("FALSE"));
}

void AUTBGPlayerController::ServerTryMove_Implementation(ABoard* InBoard, APawnBase* Unit, FIntPoint Target)
{
	if (const AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		if (GS->IsGameOver()) { UE_LOG(LogUTBGPC, Verbose, TEXT("[RPC][Move] BLOCK: GameOver")); return; }
	}

	UE_LOG(LogUTBGPC, Warning, TEXT("[RPC][Server_TryMove] PC=%s Board=%s Unit=%s From=%s -> To=%s HasAuth=%d"),
		*Obj(this), *Obj(InBoard), *Obj(Unit),
		Unit ? *Pt(Unit->GetGridCoord()) : TEXT("(null)"),
		*Pt(Target), HasAuthority());

	if (!InBoard || !Unit)
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> FAIL: null param (Board/Unit)"));
		return;
	}

	// 컨텍스트(턴/AP/팀) 출력
	if (const AUTBGGameState* GS = GetWorld()->GetGameState<AUTBGGameState>())
	{
		const ETeam TurnTeam = GS->CurrentTurnTeam;
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> GS: Turn=%s AP=%d | UnitTeam=%s"),
			TeamToStr(TurnTeam), GS->GetCurrentAP(),
			TeamToStr(UTeamUtils::GetActorTeam(Unit)));
	}

	const bool bMoved = InBoard->TryMoveUnit(Unit, Target);
	UE_LOG(LogUTBGPC, Warning, TEXT("  -> TryMoveUnit = %s"), bMoved ? TEXT("TRUE") : TEXT("FALSE"));
}

void AUTBGPlayerController::ServerEndTurn_Implementation()
{
	if (const AUTBGGameState* GS0 = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		if (GS0->IsGameOver()) { UE_LOG(LogUTBGPC, Verbose, TEXT("[RPC][EndTurn] BLOCK: GameOver")); return; }
	}


	if (AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		const ETeam MyTeam = UTeamUtils::GetActorTeam(this);
		if (GS->IsTeamTurn(MyTeam))
		{
			GS->EndTurn();
		}
	}
}

void AUTBGPlayerController::ServerTrySkill_Self_Implementation(APawnBase* User, FName SkillId, FActionTarget Target)
{
	if (const AUTBGGameState* GSx = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		if (GSx->IsGameOver()) { UE_LOG(LogUTBGPC, Verbose, TEXT("[RPC][SkillSelf] BLOCK: GameOver")); return; }
	}

	UE_LOG(LogUTBGPC, Warning, TEXT("[RPC][ServerTryUseSkillSelf] PC=%s User=%s SkillId=%s HasAuth=%d"),
		*Obj(this), *Obj(User), *SkillId.ToString(), HasAuthority());

	// 0) 기본 방어
	if (!IsValid(User)) return;

	AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr;
	if (!GS)
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> FAIL: GameState null"));
		return;
	}

	const ETeam PcTeam = UTeamUtils::GetActorTeam(this);
	const ETeam UnitTeam = UTeamUtils::GetActorTeam(User);

	const bool bTurnOK = GS->IsTeamTurn(PcTeam); 
	const bool bTeamOK = (UnitTeam == PcTeam) && (PcTeam != ETeam::ET_NoTeam);

	UE_LOG(LogUTBGPC, Warning, TEXT("  -> Guards: TurnOK=%d TeamOK=%d (You=%d Unit=%d) Resolving=%d"),
		bTurnOK ? 1 : 0, bTeamOK ? 1 : 0, (int32)PcTeam, (int32)UnitTeam, GS->bResolving ? 1 : 0);

	if (!bTurnOK || !bTeamOK || GS->bResolving) return;

	UUnitSkillsComponent* Skills = User->FindComponentByClass<UUnitSkillsComponent>();
	if (!Skills)
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> FAIL: User has no UnitSkillsComponent"));
		return;
	}

	USkillData* Data = Skills->FindSkillById(SkillId);
	if (!Data)
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> FAIL: Skill not found: %s"), *SkillId.ToString());
		return;
	}

	if (Data->TargetMode != ESkillTargetMode::Self)
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> FAIL: Skill is not Self-target mode"));
		return;
	}

	FText Reason;
	if (!Skills->CanUseByCooldownOnly(Data, Reason))
	{
		UE_LOG(LogUTBGPC, Verbose, TEXT("  -> BLOCK: Cooldown: %s"), *Reason.ToString());
		return;
	}

	GS->SetResolving(true);
	Skills->StartCooldown(Data);

	if (Data->CastMontage)
	{
		User->MulticastPlaySkillMontage(Data->CastMontage, Data->CastSection);
	}

	const float DelaySec = FMath::Max(0.f, Data->HitTimeSec);

	FTimerDelegate D = FTimerDelegate::CreateUObject(
		this, &AUTBGPlayerController::ServerApplySkillEffect_Self, User, Data
	);
	GetWorldTimerManager().SetTimer(SkillApplyTimerHandle, D, DelaySec, false);

	UE_LOG(LogUTBGPC, Verbose, TEXT("  -> Scheduled Self Hit in %.2fs"), DelaySec);
}

void AUTBGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogUTBGPC, Warning, TEXT("[BeginPlay] PC=%s NetMode=%s HasAuth=%d LocalPlayer=%s"),
		*Obj(this), NetModeToStr(GetWorld()), HasAuthority(), *Obj(GetLocalPlayer()));

	/* Enhanced Input Subsystem에 Mapping Context 추가 */
	if (UEnhancedInputLocalPlayerSubsystem* SubSys =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		SubSys->AddMappingContext(UTBGMappingContext, 0);
	}
	else
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> WARN: EnhancedInputLocalPlayerSubsystem not found"));
	}

	// 보드 캐시
	Board = Cast<ABoard>(UGameplayStatics::GetActorOfClass(GetWorld(), ABoard::StaticClass()));
	UE_LOG(LogUTBGPC, Warning, TEXT("  -> Cached Board = %s"), *Obj(Board.Get()));

	// 마우스
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	BindToGameState();
	if (!GetWorld()->GetGameState())
	{
		GetWorldTimerManager().SetTimer(GSBindRetryHandle, this, &AUTBGPlayerController::BindToGameState, 0.25f, true);
	}
}

void AUTBGPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		GS->OnMatchEnded.RemoveDynamic(this, &AUTBGPlayerController::HandleMatchEnded);
	}
	GetWorldTimerManager().ClearTimer(GSBindRetryHandle);

	Super::EndPlay(EndPlayReason);
}

void AUTBGPlayerController::BindToGameState()
{
	if (AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		GS->OnMatchEnded.AddUniqueDynamic(this, &AUTBGPlayerController::HandleMatchEnded);

		// 이미 끝난 판에 접속했을 가능성 동기화
		if (GS->IsGameOver())
		{
			HandleMatchEnded(GS->WinningTeam, GS->MatchEndReason);
		}
		GetWorldTimerManager().ClearTimer(GSBindRetryHandle);
	}
}

// ===== 포제션 =====
void AUTBGPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	UE_LOG(LogUTBGPC, Warning, TEXT("[OnPossess] PC=%s Pawn=%s Team=%s"),
		*Obj(this), *Obj(InPawn), TeamToStr(GetTeam()));
}

void AUTBGPlayerController::OnUnPossess()
{
	UE_LOG(LogUTBGPC, Warning, TEXT("[OnUnPossess] PC=%s Pawn=%s"),
		*Obj(this), *Obj(GetPawn()));
	Super::OnUnPossess();
}

void AUTBGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(IA_LeftClick, ETriggerEvent::Started, this, &AUTBGPlayerController::OnLeftClick);
		UE_LOG(LogUTBGPC, Warning, TEXT("[SetupInput] BindAction IA_LeftClick -> OnLeftClick"));
	}
	else
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("[SetupInput] WARN: EnhancedInputComponent not found"));
	}
}

void AUTBGPlayerController::OnLeftClick(const FInputActionValue&)
{
	if (const AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		if (GS->IsGameOver())
		{
			UE_LOG(LogUTBGPC, Verbose, TEXT("[Click] BLOCK: GameOver"));
			return;
		}
	}
	FHitResult Hit;
	// 가시성 채널로 커서 아래 라인트레이스
	const bool bHit =
		GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit);

	UE_LOG(LogUTBGPC, Warning, TEXT("[Click] PC=%s bHit=%d Actor=%s Loc=%s"),
		*Obj(this), bHit, *Obj(Hit.GetActor()),
		*Hit.ImpactPoint.ToString());

	if (!bHit) return;

	if (ATileActor* Tile = Cast<ATileActor>(Hit.GetActor()))
	{
		const FIntPoint Grid = Tile->GetGridCoord();
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> Tile=%s Grid=%s Board=%s"),
			*Obj(Tile), *Pt(Grid), *Obj(Board.Get()));

		if (Board.IsValid() && IsValid(Tile))
		{
			// 클릭 이벤트를 보드로 전달
			Board->HandleTileClicked(Tile);
			UE_LOG(LogUTBGPC, Warning, TEXT("  -> Dispatched to Board::HandleTileClicked"));
		}
		else
		{
			UE_LOG(LogUTBGPC, Warning, TEXT("  -> FAIL: Board invalid or Tile invalid"));
		}
	}
	else
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("  -> Not a TileActor, ignored."));
	}
}

void AUTBGPlayerController::ServerApplySkillEffect_Self(APawnBase* User, USkillData* Data)
{
	AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr;

	UE_LOG(LogUTBGPC, Warning, TEXT("[SelfHit] APPLY: User=%s Skill=%s Shield=%.0f/%.0f"),
		*Obj(User), *Obj(Data), User ? User->Shield : -1.f, User ? User->MaxShield : -1.f);

	if (!IsValid(User) || !IsValid(Data))
	{
		if (GS) GS->SetResolving(false);
		UE_LOG(LogUTBGPC, Warning, TEXT("[SelfHit] FAIL: Invalid User/Data, unlocking resolving"));
		return;
	}

	const float Missing = FMath::Max(0.f, User->MaxShield - User->Shield);
	if (Missing > 0.f)
	{
		User->AddShield(Missing);
		UE_LOG(LogUTBGPC, Warning, TEXT("[SelfHit] Refilled Shield by %.0f -> (%.0f/%.0f)"),
			Missing, User->Shield, User->MaxShield);
	}
	else
	{
		UE_LOG(LogUTBGPC, Warning, TEXT("[SelfHit] Shield already full (%.0f/%.0f)"),
			User->Shield, User->MaxShield);
	}

	if (GS) GS->SetResolving(false);
	if (Data->bEndsTurn && GS && GS->HasAuthority())
	{
		GS->EndTurn();
	}
	GetWorldTimerManager().ClearTimer(SkillApplyTimerHandle);
}

void AUTBGPlayerController::HandleMatchEnded(ETeam Winner, FString Reason)
{
	if (!IsLocalController()) return;

	UE_LOG(LogUTBGPC, Log, TEXT("[MatchEnded] Winner=%d Reason=%s"), (int32)Winner, *Reason);

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	SetInputMode(FInputModeUIOnly());
	bShowMouseCursor = true;

	if (!EndGameWidget && EndGameWidgetClass)
	{
		EndGameWidget = CreateWidget<UEndGameWidget>(this, EndGameWidgetClass);
		if (EndGameWidget)
		{
			EndGameWidget->OnExitClicked.AddUniqueDynamic(this, &AUTBGPlayerController::OnEndGameExit);
			EndGameWidget->OnRematchClicked.AddUniqueDynamic(this, &AUTBGPlayerController::OnEndGameRematch);
			EndGameWidget->AddToViewport(1000);
		}
	}

	// 결과 주입
	if (EndGameWidget)
	{
		const ETeam MyTeam = UTeamUtils::GetActorTeam(this);
		EndGameWidget->InitializeWithResult(Winner, FText::FromString(Reason), MyTeam);
		EndGameWidget->SetButtonsEnabled(true);
	}
}

void AUTBGPlayerController::OnEndGameExit()
{
	UE_LOG(LogUTBGPC, Log, TEXT("[EndGame] Exit clicked"));
	// 임시: 게임 종료(에디터에서는 PIE 종료)
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AUTBGPlayerController::OnEndGameRematch()
{
	UE_LOG(LogUTBGPC, Log, TEXT("[EndGame] Rematch clicked (stub)"));
	// TODO: 서버에 리매치 RPC 추가(나중에 GameMode로 전달)
}