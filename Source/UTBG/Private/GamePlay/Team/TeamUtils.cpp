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
        // 1) ���ֿ� ���� ������ �װ� �ֿ켱 (ü�� ���� ������ �� ��)
        if (const APawnBase* Unit = Cast<const APawnBase>(Actor))
        {
            if (Unit->Team != ETeam::ET_NoTeam)
            {
                return Unit->Team;
            }
        }
        // 2) ��Ʈ�ѷ�/���� PlayerState ����
        if (const AUTBGPlayerState* PS = Cast<const AUTBGPlayerState>(ResolvePlayerState(Actor)))
        {
            return PS->GetTeam();
        }
        return ETeam::ET_NoTeam;
    }
}

ETeam UTeamUtils::GetActorTeam(const AActor* Actor) // �ִٸ� �� �Լ��� ����
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
