// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "TeamUtils.generated.h"

/**
 * 
 */

UCLASS()
class UTBG_API UTeamUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "Team")
    static ETeam GetActorTeam(const AActor* Actor);

    /** �� ���Ͱ� ���� ������ (�� �� NoTeam�̸� false) */
    UFUNCTION(BlueprintPure, Category = "Team")
    static bool AreSameTeam(const AActor* A, const AActor* B);

    /** �� ���Ͱ� ���� ������ (�� �� NoTeam�̸� false) */
    UFUNCTION(BlueprintPure, Category = "Team")
    static bool AreEnemyTeam(const AActor* A, const AActor* B);
};
