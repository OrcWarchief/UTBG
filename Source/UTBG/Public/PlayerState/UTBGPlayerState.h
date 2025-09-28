// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GamePlay/Team/Team.h"
#include "UTBGPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class UTBG_API AUTBGPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	AUTBGPlayerState();

	UFUNCTION()
	void OnRep_Team();

	// 서버에서만 호출 GetTeam SetTeam으로 교체 요망
	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetTeam(ETeam TeamToSet);

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;

protected:

public:
	UFUNCTION(BlueprintPure, Category = "Team")
	FORCEINLINE ETeam GetTeam() const { return Team; }
};
