// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/CameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "Board/Board.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

ACameraPawn::ACameraPawn()
{
    PrimaryActorTick.bCanEverTick = false;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    RootComponent = SpringArm;
    SpringArm->bDoCollisionTest = false;
    SpringArm->SetUsingAbsoluteRotation(true);
    SpringArm->TargetArmLength = 1500.f;
    SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); // ���� ����

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);


    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->MaxSpeed = 10000.f; // ���������� ���, �츮�� AddActorOffset�� �̵�
}

void ACameraPawn::BeginPlay()
{
    Super::BeginPlay();
    Board = Cast<ABoard>(UGameplayStatics::GetActorOfClass(GetWorld(), ABoard::StaticClass()));
}

void ACameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(IA_Move,         ETriggerEvent::Triggered,   this, &ACameraPawn::Move);
        EnhancedInputComponent->BindAction(IA_Zoom,         ETriggerEvent::Triggered,   this, &ACameraPawn::Zoom);
        EnhancedInputComponent->BindAction(IA_RightClick,   ETriggerEvent::Started,     this, &ACameraPawn::RightClickStartPressed);
        EnhancedInputComponent->BindAction(IA_RightClick,   ETriggerEvent::Completed,   this, &ACameraPawn::RightClickEndPressed);
        EnhancedInputComponent->BindAction(IA_Pan,          ETriggerEvent::Triggered,   this, &ACameraPawn::Pan);
    }
}

void ACameraPawn::Move(const FInputActionValue& Value)
{
    const FVector2D v = Value.Get<FVector2D>();
    if (v.IsNearlyZero()) return;

    // ī�޶��� ���� ���� ��/�� ����
    const float dt = GetWorld()->GetDeltaSeconds();
    const FRotator YawOnly(0.f, GetActorRotation().Yaw, 0.f);
    const FVector Fwd = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

    FVector delta = (Fwd * v.Y + Right * v.X) * MoveSpeed * dt;
    AddActorWorldOffset(delta, true);
    ClampToBoard();
}

void ACameraPawn::Zoom(const FInputActionValue& Value)
{
    const float wheel = Value.Get<float>();  // +��, -�Ʒ� (������ ������ ���� �ݴ��� �� ����)
    SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength - wheel * ZoomSpeed, MinArm, MaxArm);
    ClampToBoard();
}

void ACameraPawn::Pan(const FInputActionValue& Value)
{
    if (!bDragging) return;
    const FVector2D d = Value.Get<FVector2D>(); // ���콺 ��Ÿ(�ȼ�)
    if (d.IsNearlyZero()) return;

    const FRotator YawOnly(0.f, GetActorRotation().Yaw, 0.f);
    const FVector Fwd = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

    // ȭ�鿡�� �巡���� ������ ���� ����� Shift
    FVector delta = (-Right * d.X + -Fwd * d.Y) * PanSpeed;
    AddActorWorldOffset(delta, true);
    ClampToBoard();
}

void ACameraPawn::ClampToBoard()
{
    if (!Board.IsValid()) return;

    const FBox box = Board->GetComponentsBoundingBox(true);
    const float Margin = 200.f;

    FVector loc = GetActorLocation();
    loc.X = FMath::Clamp(loc.X, box.Min.X - Margin, box.Max.X + Margin);
    loc.Y = FMath::Clamp(loc.Y, box.Min.Y - Margin, box.Max.Y + Margin);
    SetActorLocation(loc);
}

