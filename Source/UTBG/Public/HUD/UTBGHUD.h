// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GamePlay/Team/Team.h"
#include "UTBGHUD.generated.h"

class UUserOverlay;
class APawnBase;
class ABoard;
class USkillData;

/**
 * 
 */
UCLASS()
class UTBG_API AUTBGHUD : public AHUD
{
	GENERATED_BODY()
	
public:
    virtual void BeginPlay() override;

    UFUNCTION() 
    void HandleResolvingChanged(bool bResolving);
protected:
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserOverlay> OverlayClass;

    UPROPERTY() UUserOverlay* Overlay = nullptr;

    // GameState �̺�Ʈ ����
    UFUNCTION() void HandleTeamAPChanged(ETeam Team, int32 NewAP);
    UFUNCTION() void HandleTurnChanged(ETeam NewTurn);
    UFUNCTION() void HandleMatchEnded(ETeam WinningTeam, FString Reason);
    UFUNCTION() void HandleSkillChosen(APawnBase* User, FName SkillId);
    UFUNCTION() void HandleUnitSelected(APawnBase* Unit);

    // ��ư Ŭ��
    UFUNCTION() void OnEndTurnClicked();

    // �ʱ� ����Ʈ
    void InitialRefresh();

    // ���� �� ���ϱ�
    ETeam GetLocalTeam() const;

    UPROPERTY() ABoard* CachedBoard = nullptr;
};
