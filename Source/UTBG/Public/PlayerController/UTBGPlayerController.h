// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"  
#include "GenericTeamAgentInterface.h"
#include "UTBGPlayerController.generated.h"

class APawnBase;
class AUTBGGameState;
class UUserWidget;
class UInputMappingContext;
class UInputAction;
class ABoard;
class USkillData;
class UEndGameWidget;

USTRUCT(BlueprintType)
struct UTBG_API FActionTarget
{
    GENERATED_BODY()

    // 앞으로 확장용 (Unit/Tile)
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<APawnBase> TargetUnit = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FIntPoint TargetTile = FIntPoint::ZeroValue;

    // 편의 생성자
    static FActionTarget None() { return FActionTarget(); }
    static FActionTarget MakeUnit(APawnBase* U) { FActionTarget T; T.TargetUnit = U; return T; }
    static FActionTarget MakeTile(FIntPoint P) { FActionTarget T; T.TargetTile = P; return T; }
};



UCLASS()
class UTBG_API AUTBGPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
public:
    virtual void SetupInputComponent() override;
    UFUNCTION(BlueprintCallable, Category = "Team")
    ETeam GetTeam() const;

    UFUNCTION(Server, Reliable)
    void ServerSetTeam(ETeam NewTeam);

    UFUNCTION(Server, Reliable)
    void ServerTryAttack(ABoard* InBoard, APawnBase* Attacker, APawnBase* Target);

    UFUNCTION(Server, Reliable)
    void ServerTryMove(ABoard* InBoard, APawnBase* Unit, FIntPoint Target);

    UFUNCTION(Server, Reliable)
    void ServerTrySkill_Self(APawnBase* User, FName SkillId, FActionTarget Target);

    UFUNCTION(Server, Reliable)
    void ServerEndTurn();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;


    /* 바인딩 함수 */
    void OnLeftClick(const FInputActionValue& Value);

    /* 에디터에서 지정 */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputMappingContext* UTBGMappingContext;
    UPROPERTY(EditDefaultsOnly, Category = "Input") 
    UInputAction* IA_LeftClick;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> EndGameWidgetClass;

private:
    TWeakObjectPtr<ABoard> Board;
    FTimerHandle SkillApplyTimerHandle;

    void BindToGameState();
    FTimerHandle GSBindRetryHandle;
    UPROPERTY() 
    UEndGameWidget* EndGameWidget = nullptr;

    UFUNCTION() 
    void OnEndGameExit();
    UFUNCTION() 
    void OnEndGameRematch();

    UFUNCTION() 
    void HandleMatchEnded(ETeam Winner, FString Reason);

    UFUNCTION()
    void ServerApplySkillEffect_Self(APawnBase* User, USkillData* Data);
};
