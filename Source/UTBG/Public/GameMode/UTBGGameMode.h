// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "UTBGGameMode.generated.h"

class APawnBase;

UCLASS()
class UTBG_API AUTBGGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AUTBGGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:
	virtual void HandleMatchHasStarted() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	int32 ConnectedPlayers = 0;
	int32 MaxTeams = 2;
	
	bool bGameStarted = false;
	bool bGameEnded = false;

	void StartGame();
	void EndGame(uint8 WinningTeam, const FString& Reason);

	// 로그용 함수
	void LogGameStatus() const;

	void BindExistingPawns();
	void BindToPawn(APawnBase* Pawn);
	void OnActorSpawned(AActor* SpawnedActor);

	UFUNCTION()
	void HandlePawnDied(APawnBase* Pawn);
	void ResolveEndCheck();

	TArray<TWeakObjectPtr<APawnBase>> TrackedKingsBlue;
	TArray<TWeakObjectPtr<APawnBase>> TrackedKingsRed;

	FTimerHandle EndCheckTimerHandle;
	bool bEndCheckScheduled = false;
	FDelegateHandle ActorSpawnedHandle;
};
