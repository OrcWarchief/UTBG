// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/UTBGPlayerState.h"
#include "GameState/UTBGGameState.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/PawnBase.h"
#include "Engine/Engine.h"

AUTBGPlayerState::AUTBGPlayerState()
{
	SetReplicates(true);
	NetUpdateFrequency = 10.0f;			// 팀 정보는 자주 업데이트 X
}

void AUTBGPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;

	APawnBase* PawnBase = Cast<APawnBase>(GetPawn());
	if (PawnBase)
	{
		PawnBase->SetTeamColor(Team);
	}
	if (auto* UTBGGameState = GetWorld()->GetGameState<AUTBGGameState>())
	{
		UTBGGameState->NotifyPlayerTeamChanged(this);
	}
	ForceNetUpdate();
}

void AUTBGPlayerState::OnRep_Team()
{
	APawnBase* PawnBase = Cast<APawnBase>(GetPawn());
	if (PawnBase)
	{
		PawnBase->SetTeamColor(Team);
	}
}

void AUTBGPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTBGPlayerState, Team);
}
