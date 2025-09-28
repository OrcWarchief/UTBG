// Fill out your copyright notice in the Description page of Project Settings.


#include "GamePlay/Team/TeamUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "PlayerState/UTBGPlayerState.h" 
#include "Pawn/PawnBase.h"

namespace
{
    FORCEINLINE const APlayerState* ResolvePlayerState(const AActor* Actor)
    {
        if (!Actor) return nullptr;
        if (const APawn* P = Cast<const APawn>(Actor))      return P->GetPlayerState();
        if (const AController* C = Cast<const AController>(Actor)) return C->PlayerState.Get();
        return nullptr;
    }

    FORCEINLINE ETeam GetTeamFromActor(const AActor* Actor)
    {
        // 1) 유닛에 팀이 있으면 그걸 최우선 (체스 말은 포제션 안 됨)
        if (const APawnBase* Unit = Cast<const APawnBase>(Actor))
        {
            if (Unit->Team != ETeam::ET_NoTeam)
            {
                return Unit->Team;
            }
        }
        // 2) 컨트롤러/폰의 PlayerState 경유
        if (const AUTBGPlayerState* PS = Cast<const AUTBGPlayerState>(ResolvePlayerState(Actor)))
        {
            return PS->GetTeam();
        }
        return ETeam::ET_NoTeam;
    }
}

ETeam UTeamUtils::GetActorTeam(const AActor* Actor) // 있다면 이 함수도 유지
{
    return GetTeamFromActor(Actor);
}

bool UTeamUtils::AreSameTeam(const AActor* A, const AActor* B)
{
    const ETeam TA = GetTeamFromActor(A);
    const ETeam TB = GetTeamFromActor(B);
    return (TA != ETeam::ET_NoTeam) && (TA == TB);
}

bool UTeamUtils::AreEnemyTeam(const AActor* A, const AActor* B)
{
    const ETeam TA = GetTeamFromActor(A);
    const ETeam TB = GetTeamFromActor(B);
    return (TA != ETeam::ET_NoTeam) && (TB != ETeam::ET_NoTeam) && (TA != TB);
}
