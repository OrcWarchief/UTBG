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
    * 팀 (Replicated) 
    */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Teams")
    TArray<TObjectPtr<AUTBGPlayerState>> RedTeam;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Teams")
    TArray<TObjectPtr<AUTBGPlayerState>> BlueTeam;

    /**
    * 턴 / AP (Replicated)
    */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn")
    int32 MaxAPPerTurn = 5;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentTurnTeam, BlueprintReadOnly, Category = "Turn")
    ETeam CurrentTurnTeam = ETeam::ET_RedTeam;    // 기본 선공 : 레드

    UPROPERTY(ReplicatedUsing = OnRep_CurrentTeamAP, BlueprintReadOnly, Category = "Turn")
    int32 CurrentTeamAP = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Resolving, BlueprintReadOnly, Category = "Turn")
    bool bResolving = false;                    // 현재 액션(스킬/공격) 후 쿨다운 해소 중인지

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Turn")
    int32 TurnIndex = 0;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTeamAPChanged OnTeamAPChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTurnChanged OnTurnChanged;

    UPROPERTY(ReplicatedUsing = OnRep_MatchResult, BlueprintReadOnly, Category = "Match")
    bool bMatchEnded = false; //매치가 종료되었는가 (Rep)

    UPROPERTY(ReplicatedUsing = OnRep_MatchResult, BlueprintReadOnly, Category = "Match")
    ETeam WinningTeam = ETeam::ET_NoTeam; //  우승 팀 (Rep) 

    /** 종료 사유(간단 문자열, Rep) */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    FString MatchEndReason;

    /** 클라/HUD용 이벤트: 매치 종료 알림 */
    UPROPERTY(BlueprintAssignable, Category = "Match|Events")
    FOnMatchEnded OnMatchEnded;

    /** 서버만 호출: 결과 설정 + 복제 */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
    void ServerSetMatchResult(ETeam Winner, const FString& Reason);

    /** 블루프린트/코드에서 게임 종료 여부 확인 */
    UFUNCTION(BlueprintPure, Category = "Match")
    FORCEINLINE bool IsGameOver() const { return bMatchEnded; }

    /**
    * Public API (Board/Controller/UI에서 사용)
    */

    //  TODO : 나중에 IsTeamTurn 이랑 합치기
    UFUNCTION(BlueprintPure, Category = "Turn")
    bool IsActorTurn(const AActor* Actor) const;

    // 해당 코스트를 지불할 수 있는가? (턴/잔여AP 동시 판정)
    UFUNCTION(BlueprintPure, Category = "Turn")
    bool HasAPForActor(const AActor* Actor, int32 Cost = 1) const;

    // AP 차감
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Turn")
    bool TrySpendAPForActor(AActor* Actor, int32 Cost);

    // 턴 종료
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Turn")
    void EndTurn();

    // 쿨다운 중 일시 스킬 잠금
    UFUNCTION(BlueprintCallable, Category = "Turn")
    void SetResolving(bool bNew);

    /**
    * 팀 캐시 유지
    */
    // 특정 PlayerState의 팀이 변동되면 호출
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Teams")
    void NotifyPlayerTeamChanged(AUTBGPlayerState* PS);

    // PlayerArray 기준으로 캐시 전체 재구성(안전망)
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Teams")
    void RefreshTeamsFromPlayerArray();

    // 편의 : 팀 인원 수
    UFUNCTION(BlueprintPure, Category = "Teams")
    int32 GetTeamPlayerCount(ETeam Team) const;

protected:
    /**
    * OnRep : 클라 UI/하이라이트 갱신 트리거
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
