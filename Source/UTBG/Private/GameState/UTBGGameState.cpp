// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/UTBGGameState.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/UTBGPlayerState.h"
#include "GamePlay/Team/TeamUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTBGGameState, Log, All);

AUTBGGameState::AUTBGGameState()
{
    // GameState는 기본적으로 복제됨
}

void AUTBGGameState::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        // 첫 턴, AP
        CurrentTurnTeam = ETeam::ET_RedTeam;
        CurrentTeamAP = MaxAPPerTurn;

        // 서버 로컬 최초 방송 (리슨서버 HUD 초기화용)
        OnTurnChanged.Broadcast(CurrentTurnTeam);
        OnTeamAPChanged.Broadcast(CurrentTurnTeam, CurrentTeamAP);
    }
}

void AUTBGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AUTBGGameState, RedTeam);
    DOREPLIFETIME(AUTBGGameState, BlueTeam);

    DOREPLIFETIME(AUTBGGameState, CurrentTurnTeam);
    DOREPLIFETIME(AUTBGGameState, CurrentTeamAP);
    DOREPLIFETIME(AUTBGGameState, TurnIndex);
    DOREPLIFETIME(AUTBGGameState, bResolving);

    DOREPLIFETIME(AUTBGGameState, bMatchEnded);
    DOREPLIFETIME(AUTBGGameState, WinningTeam);
    DOREPLIFETIME(AUTBGGameState, MatchEndReason);
}

bool AUTBGGameState::IsActorTurn(const AActor* Actor) const
{
    if (!Actor || bMatchEnded) return false;
    const ETeam ActorTeam = UTeamUtils::GetActorTeam(Actor);
    return ActorTeam != ETeam::ET_NoTeam && ActorTeam == CurrentTurnTeam;
}

bool AUTBGGameState::HasAPForActor(const AActor* Actor, int32 Cost) const
{
    if (bMatchEnded) return false;
    if (Cost <= 0) return true;
    if (!IsActorTurn(Actor)) return false;
    return CurrentTeamAP >= Cost;
}

bool AUTBGGameState::TrySpendAPForActor(AActor* Actor, int32 Cost)
{
    if (!HasAuthority()) return false;
    if (bMatchEnded) return false;
    if (!IsValid(Actor)) return false;
    if (Cost <= 0) return true;

    if (!IsActorTurn(Actor))
    {
        UE_LOG(LogUTBGGameState, Verbose, TEXT("[AP] Not your turn."));
        return false;
    }
    if (CurrentTeamAP < Cost)
    {
        UE_LOG(LogUTBGGameState, Verbose, TEXT("[AP] Not enough AP. Has=%d Need=%d"), CurrentTeamAP, Cost);
        return false;
    }

    CurrentTeamAP -= Cost;
    CurrentTeamAP = FMath::Max(0, CurrentTeamAP);

    UE_LOG(LogUTBGGameState, Log, TEXT("[AP] Spent %d -> AP=%d"), Cost, CurrentTeamAP);

    if (CurrentTeamAP <= 0)
    {
        UE_LOG(LogUTBGGameState, Log, TEXT("[TURN] Auto EndTurn: Team=%d AP=%d"), (int32)CurrentTurnTeam, CurrentTeamAP);
        EndTurn(); // 다음 팀으로 전환 + AP 리필
        return true;
    }

    // AP만 감소한 경우 알림
    OnTeamAPChanged.Broadcast(CurrentTurnTeam, CurrentTeamAP);
    return true;
}

void AUTBGGameState::EndTurn()
{
    if (!HasAuthority() || bMatchEnded) return;

    const ETeam Prev = CurrentTurnTeam;
    CurrentTurnTeam = GetOppositeTeam(CurrentTurnTeam);
    ++TurnIndex;

    CurrentTeamAP = MaxAPPerTurn;

    OnTurnChanged.Broadcast(CurrentTurnTeam);
    OnTeamAPChanged.Broadcast(CurrentTurnTeam, CurrentTeamAP);

    UE_LOG(LogUTBGGameState, Log, TEXT("[TURN] %d -> %d | AP=%d | TurnIndex=%d"),
        (int32)Prev, (int32)CurrentTurnTeam, CurrentTeamAP, TurnIndex);
}

void AUTBGGameState::SetResolving(bool bNew)
{
    if (!HasAuthority()) return;
    if (bMatchEnded) bNew = false;
    if (bResolving == bNew) return;
    bResolving = bNew;
    UE_LOG(LogUTBGGameState, Log, TEXT("[Resolving] -> %s"), bResolving ? TEXT("TRUE") : TEXT("FALSE"));
    OnRep_Resolving();
}

void AUTBGGameState::OnRep_CurrentTurnTeam()
{
    OnTurnChanged.Broadcast(CurrentTurnTeam);
}

void AUTBGGameState::OnRep_CurrentTeamAP()
{
    OnTeamAPChanged.Broadcast(CurrentTurnTeam, CurrentTeamAP);
}

void AUTBGGameState::OnRep_Resolving()
{
    UE_LOG(LogUTBGGameState, Log, TEXT("[OnRep_Resolving] %s"), bResolving ? TEXT("TRUE") : TEXT("FALSE"));
    // TODO: HUD에 잠금/해제 전달 (IsEnabled 토글)
}

void AUTBGGameState::ServerSetMatchResult(ETeam Winner, const FString& Reason)
{
    if (!HasAuthority()) return;
    if (bMatchEnded) return;

    bMatchEnded = true;
    WinningTeam = Winner;
    MatchEndReason = Reason;

    // 종료 즉시 진행 중 액션 잠금 해제(입력은 PC/HUD에서 잠금)
    bResolving = false;

    // 서버에서도 즉시 통지
    OnRep_MatchResult();

    UE_LOG(LogUTBGGameState, Log, TEXT("[MATCH] End -> Winner=%d, Reason=%s"), (int32)Winner, *Reason);

    // 복제 빠르게
    ForceNetUpdate();
}

void AUTBGGameState::OnRep_MatchResult()
{
    if (bMatchEnded)
    {
        OnMatchEnded.Broadcast(WinningTeam, MatchEndReason);
    }
}

void AUTBGGameState::NotifyPlayerTeamChanged(AUTBGPlayerState* PS)
{
    if (!HasAuthority() || !IsValid(PS)) return;

    RemoveFromTeams(PS);
    AddToTeamArray(PS, PS->GetTeam());

    ForceNetUpdate(); // 빠른 전파
}

void AUTBGGameState::RefreshTeamsFromPlayerArray()
{
    if (!HasAuthority()) return;

    RedTeam.Reset();
    BlueTeam.Reset();

    for (APlayerState* Base : PlayerArray)
    {
        if (auto* PS = Cast<AUTBGPlayerState>(Base))
        {
            AddToTeamArray(PS, PS->GetTeam());
        }
    }
    ForceNetUpdate();
}

int32 AUTBGGameState::GetTeamPlayerCount(ETeam Team) const
{
    int32 Count = 0;
    for (APlayerState* Base : PlayerArray)
    {
        if (const auto* PS = Cast<AUTBGPlayerState>(Base))
        {
            if (PS->GetTeam() == Team) ++Count;
        }
    }
    return Count;
}

void AUTBGGameState::AddToTeamArray(AUTBGPlayerState* PS, ETeam Team)
{
    if (!IsValid(PS)) return;

    if (Team == ETeam::ET_RedTeam)        RedTeam.AddUnique(PS);
    else if (Team == ETeam::ET_BlueTeam)  BlueTeam.AddUnique(PS);
    // ET_NoTeam 은 미포함
}

void AUTBGGameState::RemoveFromTeams(AUTBGPlayerState* PS)
{
    RedTeam.Remove(PS);
    BlueTeam.Remove(PS);
}

ETeam AUTBGGameState::GetOppositeTeam(ETeam T)
{
    if (T == ETeam::ET_RedTeam)  return ETeam::ET_BlueTeam;
    if (T == ETeam::ET_BlueTeam) return ETeam::ET_RedTeam;
    return ETeam::ET_RedTeam; // 기본값
}
