// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/Team/Team.h"
#include "CameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class ABoard;
class UFloatingPawnMovement;

UCLASS()
class UTBG_API ACameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ACameraPawn();
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Team")
	ETeam Team = ETeam::ET_NoTeam;

	UFUNCTION(BlueprintCallable, Category = "UI|Widget Scale")
	void UpdateWidgetScaleForAllPawns();

protected:
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm = nullptr;
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera = nullptr;
	UPROPERTY(VisibleAnywhere) 
	UFloatingPawnMovement* Movement = nullptr;

	// Enhanced Input
	UPROPERTY(EditDefaultsOnly, Category = "Input") 
	const UInputAction* IA_Move = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Input") 
	const UInputAction* IA_Zoom = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Input") 
	const UInputAction* IA_RightClick = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Input") 
	const UInputAction* IA_Pan = nullptr;

	UFUNCTION() 
	void Move(const struct FInputActionValue& Value);
	UFUNCTION() 
	void Zoom(const struct FInputActionValue& Value);
	UFUNCTION() 
	void RightClickStartPressed()	{ bDragging = true; }
	UFUNCTION() 
	void RightClickEndPressed()		{ bDragging = false; }
	UFUNCTION() 
	void Pan(const struct FInputActionValue& Value);

	bool bDragging = false;

	UPROPERTY(EditAnywhere, Category = "Camera") 
	float MoveSpeed = 2000.f;
	UPROPERTY(EditAnywhere, Category = "Camera") 
	float PanSpeed = 1.0f;   // ¸¶¿ì½º µå·¡±× ¹Î°¨µµ
	UPROPERTY(EditAnywhere, Category = "Camera") 
	float ZoomSpeed = 500.f;
	UPROPERTY(EditAnywhere, Category = "Camera") 
	float MinArm = 600.f;
	UPROPERTY(EditAnywhere, Category = "Camera") 
	float MaxArm = 3000.f;

	// À§Á¬ ½ºÄÉÀÏ Æ©´×°ª
	UPROPERTY(EditAnywhere, Category = "UI|Widget Scale")
	float MinWidgetScale = 0.05f;   // ÁÜ ÀÎ
	UPROPERTY(EditAnywhere, Category = "UI|Widget Scale")
	float MaxWidgetScale = 1.00f;   // ÁÜ ¾Æ¿ô
	UPROPERTY(EditAnywhere, Category = "UI|Widget Scale")
	float ScaleLerpSpeed = 0.f;

	// LOD thresholds  Arm > LOD0Arm => LOD0, Arm > LOD1Arm => LOD1, else LOD2.
	UPROPERTY(EditAnywhere, Category = "UI|Widget LOD") float LOD0Arm = 2400.f; // far  -> bars only
	UPROPERTY(EditAnywhere, Category = "UI|Widget LOD") float LOD1Arm = 1400.f; // mid  -> + HP text
	// near (< LOD1Arm)                     -> full (shield row too)

	// Reduce spam updates
	UPROPERTY(EditAnywhere, Category = "UI|Widget") float NotifyDeltaThreshold = 5.f;

	float LastArmLengthNotified = -1.f;
	int32 LastLODNotified = INDEX_NONE;

	TWeakObjectPtr<ABoard> Board;

	void ClampToBoard();

	float ComputeTargetScale(float Arm) const;
	int32 ComputeLOD(float Arm) const;
public:	

};
