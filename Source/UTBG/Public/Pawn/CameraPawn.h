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

protected:
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm = nullptr;
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera = nullptr;
	UPROPERTY(VisibleAnywhere) 
	class UFloatingPawnMovement* Movement = nullptr;

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
	float PanSpeed = 1.0f;   // 마우스 드래그 민감도
	UPROPERTY(EditAnywhere, Category = "Camera") 
	float ZoomSpeed = 500.f;
	UPROPERTY(EditAnywhere, Category = "Camera") 
	float MinArm = 600.f;
	UPROPERTY(EditAnywhere, Category = "Camera") 
	float MaxArm = 3000.f;

	TWeakObjectPtr<ABoard> Board;

	void ClampToBoard();

public:	

};
