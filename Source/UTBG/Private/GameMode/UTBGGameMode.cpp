// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/UTBGGameMode.h"
#include "PlayerState/UTBGPlayerState.h"
#include "GameState/UTBGGameState.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Pawn/PawnBase.h"
#include "GamePlay/Team/Team.h" 

AUTBGGameMode::AUTBGGameMode()
{
	ConnectedPlayers = 0;
	bGameStarted = false;
	bGameEnded = false;
}

void AUTBGGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	AUTBGGameState* BGameState = Cast<AUTBGGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		AUTBGPlayerState* BPState = NewPlayer->GetPlayerState<AUTBGPlayerState>();
		if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
			{
				BGameState->RedTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_RedTeam);
				UE_LOG(LogTemp, Warning, TEXT("You're Red Team"));
			}
			else
			{
				BGameState->BlueTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_BlueTeam);
				UE_LOG(LogTemp, Warning, TEXT("You're Blue Team"));
			}
		}
	}
}

void AUTBGGameMode::Logout(AController* Exiting)
{
	AUTBGGameState* BGameState = Cast<AUTBGGameState>(UGameplayStatics::GetGameState(this));
	AUTBGPlayerState* BPState = Exiting->GetPlayerState<AUTBGPlayerState>();
	if (BGameState && BPState)
	{
		if (BGameState->RedTeam.Contains(BPState))
		{
			BGameState->RedTeam.Remove(BPState);
		}
		if (BGameState->BlueTeam.Contains(BPState))
		{
			BGameState->BlueTeam.Remove(BPState);
		}
	}
}

void AUTBGGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	AUTBGGameState* BGameState = Cast<AUTBGGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		for (auto PState : BGameState->PlayerArray)
		{
			AUTBGPlayerState* BPState = Cast<AUTBGPlayerState>(PState.Get());
			if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
				{
					BGameState->RedTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BGameState->BlueTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}

	BindExistingPawns();
}

void AUTBGGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("[GM] BeginPlay on %s"), *GetClass()->GetName());

	if (UWorld* World = GetWorld())
	{
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &AUTBGGameMode::OnActorSpawned));
	}

	BindExistingPawns();
}

void AUTBGGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ActorSpawnedHandle.IsValid())
		{
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
			ActorSpawnedHandle.Reset();
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AUTBGGameMode::StartGame()
{
	if (bGameStarted) return; 
	
	bGameStarted = true; 
	bGameEnded = false; 
	UE_LOG(LogTemp, Log, TEXT("AUTBGGameMode::StartGame() : Game Started!")); 
	
	if (GEngine) 
	{ 
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("AUTBGGameMode::StartGame() : Game Started!")); 
	} 
	// AGameMode�� StartMatch ȣ�� 
	StartMatch(); 
	// ���� ���� �� �߰� ���� // ��: Ÿ�̸� ����, ù �� ����, UI ������Ʈ ��
}

void AUTBGGameMode::EndGame(uint8 WinningTeam, const FString& Reason)
{
	if (bGameEnded) return;
	bGameEnded = true;

	if (AUTBGGameState* GS = Cast<AUTBGGameState>(UGameplayStatics::GetGameState(this)))
	{
		GS->ServerSetMatchResult(static_cast<ETeam>(WinningTeam), Reason);
	}

	EndMatch();

	UE_LOG(LogTemp, Log, TEXT("Game Ended! Team %d wins. Reason: %s"), WinningTeam, *Reason);
	if (GEngine)
	{
		FString Message = FString::Printf(TEXT("Team %d Wins! %s"), WinningTeam, *Reason);
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, Message);
	}
	// TODO: ��� ȭ��/���/����ġ �غ� ��
}

void AUTBGGameMode::LogGameStatus() const
{
	FString StatusMessage = FString::Printf(
		TEXT("Game Status - Players: %d/2, Started: %s, Ended: %s"), 
		ConnectedPlayers, 
		bGameStarted ? TEXT("Yes") : TEXT("No"), 
		bGameEnded ? TEXT("Yes") : TEXT("No")); 
	
	UE_LOG(LogTemp, Log, TEXT("%s"), *StatusMessage); 
	if (GEngine) 
	{ 
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, StatusMessage); 
	}
}
void AUTBGGameMode::BindExistingPawns()
{
	if (!HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<APawnBase> It(World); It; ++It)
	{
		BindToPawn(*It);
	}
}

void AUTBGGameMode::BindToPawn(APawnBase* Pawn)
{
	if (!HasAuthority() || !IsValid(Pawn)) return;

	// �̹� �ı� ������ ���� ����
	if (Pawn->IsActorBeingDestroyed()) return;

	// ��� ����ó ����(�ߺ� ���� ���� �� ���� ���ε��ǵ���)
	Pawn->OnPawnDied.AddUniqueDynamic(this, &AUTBGGameMode::HandlePawnDied);

	// ���̸� ���� ����Ʈ�� ���(�ߺ� ����)
	if (Pawn->bIsKing)
	{
		switch (Pawn->Team)
		{
		case ETeam::ET_BlueTeam:
			TrackedKingsBlue.AddUnique(Pawn);
			break;
		case ETeam::ET_RedTeam:
			TrackedKingsRed.AddUnique(Pawn);
			break;
		default:
			break;
		}
	}
}

void AUTBGGameMode::OnActorSpawned(AActor* SpawnedActor)
{
	if (!HasAuthority()) return;

	if (APawnBase* Pawn = Cast<APawnBase>(SpawnedActor))
	{
		BindToPawn(Pawn);
	}
}

void AUTBGGameMode::HandlePawnDied(APawnBase* Pawn)
{
	if (!HasAuthority() || bGameEnded || !IsValid(Pawn)) return;

	// ���� �ƴϸ� ���� ����� ����
	if (!Pawn->bIsKing) return;

	UE_LOG(LogTemp, Log, TEXT("[GM] King died: %s (Team=%d)"), *Pawn->GetName(), static_cast<int32>(Pawn->Team));

	// ���� ��� ���ɼ� �� ª�� ���� �� ����(���� �ð��� Ȯ��)
	if (!bEndCheckScheduled)
	{
		bEndCheckScheduled = true;
		// PawnBase�� �⺻ ��� ���� ����(0.5s �̻�)�� �����ؼ� 1.0s ������ ����
		GetWorldTimerManager().SetTimer(EndCheckTimerHandle, this, &AUTBGGameMode::ResolveEndCheck, 1.0f, false);
	}
}

void AUTBGGameMode::ResolveEndCheck()
{
	bEndCheckScheduled = false;

	// ��ȿ/������ ���� ���� �ִ��� Ȯ��
	auto IsTeamKingAlive = [](const TArray<TWeakObjectPtr<APawnBase>>& Kings) -> bool
		{
			for (const TWeakObjectPtr<APawnBase>& W : Kings)
			{
				const APawnBase* K = W.Get();
				if (IsValid(K) && !K->IsActorBeingDestroyed() && !K->IsDead())
				{
					return true;
				}
			}
			return false;
		};

	const bool bBlueAlive = IsTeamKingAlive(TrackedKingsBlue);
	const bool bRedAlive = IsTeamKingAlive(TrackedKingsRed);

	if (!bBlueAlive && !bRedAlive)
	{
		// ���º�(�� �� �� ����)
		EndGame(static_cast<uint8>(ETeam::ET_NoTeam), TEXT("Draw: both kings eliminated."));
		return;
	}

	if (bBlueAlive && !bRedAlive)
	{
		EndGame(static_cast<uint8>(ETeam::ET_BlueTeam), TEXT("Red king defeated."));
		return;
	}

	if (!bBlueAlive && bRedAlive)
	{
		EndGame(static_cast<uint8>(ETeam::ET_RedTeam), TEXT("Blue king defeated."));
		return;
	}

	// �� �� ��� ����ִٸ�(�� ��δ� ���� ������) ���� ����
	UE_LOG(LogTemp, Verbose, TEXT("[GM] ResolveEndCheck: both kings still alive. Continue."));
}