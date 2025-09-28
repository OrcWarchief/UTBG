// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GamePlay/Team/Team.h"  
#include "UTBGGameState.generated.h"

class AUTBGPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamAPChanged, ETeam, Team, int32, NewAP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnChanged, ETeam, NewTurnTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnded, ETeam, WinningTeam, FString, Reason);

UCLASS()
class UTBG_API AUTBGGameState : public AGameState
{
	GENERATED_BODY()

public:
    AUTBGGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    /** 
    * �� (Replicated) 
    */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Teams")
    TArray<TObjectPtr<AUTBGPlayerState>> RedTeam;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Teams")
    TArray<TObjectPtr<AUTBGPlayerState>> BlueTeam;

    /**
    * �� / AP (Replicated)
    */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn")
    int32 MaxAPPerTurn = 5;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentTurnTeam, BlueprintReadOnly, Category = "Turn")
    ETeam CurrentTurnTeam = ETeam::ET_RedTeam;    // �⺻ ���� : ����

    UPROPERTY(ReplicatedUsing = OnRep_CurrentTeamAP, BlueprintReadOnly, Category = "Turn")
    int32 CurrentTeamAP = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Resolving, BlueprintReadOnly, Category = "Turn")
    bool bResolving = false;                    // ���� �׼�(��ų/����) �� ��ٿ� �ؼ� ������

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
    int32 TurnIndex = 0;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTeamAPChanged OnTeamAPChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTurnChanged OnTurnChanged;

    UPROPERTY(ReplicatedUsing = OnRep_MatchResult, BlueprintReadOnly, Category = "Match")
    bool bMatchEnded = false; //��ġ�� ����Ǿ��°� (Rep)

    UPROPERTY(ReplicatedUsing = OnRep_MatchResult, BlueprintReadOnly, Category = "Match")
    ETeam WinningTeam = ETeam::ET_NoTeam; //  ��� �� (Rep) 

    /** ���� ����(���� ���ڿ�, Rep) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    FString MatchEndReason;

    /** Ŭ��/HUD�� �̺�Ʈ: ��ġ ���� �˸� */
    UPROPERTY(BlueprintAssignable, Category = "Match|Events")
    FOnMatchEnded OnMatchEnded;

    /** ������ ȣ��: ��� ���� + ���� */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
    void ServerSetMatchResult(ETeam Winner, const FString& Reason);

    /** �������Ʈ/�ڵ忡�� ���� ���� ���� Ȯ�� */
    UFUNCTION(BlueprintPure, Category = "Match")
    FORCEINLINE bool IsGameOver() const { return bMatchEnded; }

    /**
    * Public API (Board/Controller/UI���� ���)
    */

    //  TODO : ���߿� IsTeamTurn �̶� ��ġ��
    UFUNCTION(BlueprintPure, Category = "Turn")
    bool IsActorTurn(const AActor* Actor) const;

    // �ش� �ڽ�Ʈ�� ������ �� �ִ°�? (��/�ܿ�AP ���� ����)
    UFUNCTION(BlueprintPure, Category = "Turn")
    bool HasAPForActor(const AActor* Actor, int32 Cost = 1) const;

    // AP ����
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Turn")
    bool TrySpendAPForActor(AActor* Actor, int32 Cost);

    // �� ����
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Turn")
    void EndTurn();

    // ��ٿ� �� �Ͻ� ��ų ���
    UFUNCTION(BlueprintCallable, Category = "Turn")
    void SetResolving(bool bNew);

    /**
    * �� ĳ�� ����
    */
    // Ư�� PlayerState�� ���� �����Ǹ� ȣ��
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Teams")
    void NotifyPlayerTeamChanged(AUTBGPlayerState* PS);

    // PlayerArray �������� ĳ�� ��ü �籸��(������)
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Teams")
    void RefreshTeamsFromPlayerArray();

    // ���� : �� �ο� ��
    UFUNCTION(BlueprintPure, Category = "Teams")
    int32 GetTeamPlayerCount(ETeam Team) const;

protected:
    /**
    * OnRep : Ŭ�� UI/���̶���Ʈ ���� Ʈ����
    */
    UFUNCTION() void OnRep_CurrentTurnTeam();
    UFUNCTION() void OnRep_CurrentTeamAP();
    UFUNCTION() void OnRep_Resolving();
    UFUNCTION() void OnRep_MatchResult();

private:
    void AddToTeamArray(AUTBGPlayerState* PS, ETeam Team);
    void RemoveFromTeams(AUTBGPlayerState* PS);
    static ETeam GetOppositeTeam(ETeam T);

public:
    UFUNCTION(BlueprintPure, Category = "Turn")
    FORCEINLINE bool IsTeamTurn(ETeam Team) const { return Team != ETeam::ET_NoTeam && Team == CurrentTurnTeam; }
    UFUNCTION(BlueprintPure, Category = "Turn")
    FORCEINLINE int32 GetCurrentAP() const { return CurrentTeamAP; }
};
