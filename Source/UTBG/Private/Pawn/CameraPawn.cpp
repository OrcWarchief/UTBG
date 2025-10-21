// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/CameraPawn.h"
#include "Pawn/PawnBase.h"
#include "Board/Board.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

#include "EnhancedInputComponent.h"
#include "InputActionValue.h"

static FORCEINLINE FVector GetCamLoc(UWorld* World)
{
    if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(World, 0)) return Cam->GetCameraLocation();
    return FVector::ZeroVector;
}

ACameraPawn::ACameraPawn()
{
    PrimaryActorTick.bCanEverTick = false;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    RootComponent = SpringArm;
    SpringArm->bDoCollisionTest = false;
    SpringArm->SetUsingAbsoluteRotation(true);
    SpringArm->TargetArmLength = 1500.f;
    SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); // 고정 각도

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);


    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->MaxSpeed = 10000.f; // 내부적으로 사용, AddActorOffset로 이동
}

void ACameraPawn::BeginPlay()
{
    Super::BeginPlay();
    Board = Cast<ABoard>(UGameplayStatics::GetActorOfClass(GetWorld(), ABoard::StaticClass()));
    LastArmLengthNotified = SpringArm->TargetArmLength;
    UpdateWidgetScaleForAllPawns();
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

float ACameraPawn::ComputeTargetScale(float Arm) const
{
    // Zoom in -> bigger scale
    return FMath::GetMappedRangeValueClamped(
        FVector2D(MinArm, MaxArm),
        FVector2D(MaxWidgetScale, MinWidgetScale),
        Arm);
}

int32 ACameraPawn::ComputeLOD(float Arm) const
{
    // Farther arm => smaller detail
    if (Arm > LOD0Arm) return 0;     // far: bars only
    if (Arm > LOD1Arm) return 1;     // mid: + HP text
    return 2;                         // near: full (shield row)
}

void ACameraPawn::UpdateWidgetScaleForAllPawns()
{
    if (!IsLocallyControlled()) return;

    const float Arm = SpringArm->TargetArmLength;
    const float Scale = ComputeTargetScale(Arm);
    const int32 LOD = ComputeLOD(Arm);
    const bool  bNotify =
        (LastArmLengthNotified < 0.f) ||
        (FMath::Abs(Arm - LastArmLengthNotified) > NotifyDeltaThreshold) ||
        (LOD != LastLODNotified);

    if (!bNotify) return;

    const FVector CamLoc = GetCamLoc(GetWorld());

    for (TActorIterator<APawnBase> It(GetWorld()); It; ++It)
    {
        It->ApplyWidgetScale(Scale);
        It->ApplyWidgetLOD(LOD);
    }

    LastArmLengthNotified = Arm;
    LastLODNotified = LOD;

    UE_LOG(LogTemp, Log, TEXT("[CameraPawn] Arm=%.0f  Scale=%.2f  LOD=%d"), Arm, Scale, LOD);
}

void ACameraPawn::Move(const FInputActionValue& Value)
{
    const FVector2D v = Value.Get<FVector2D>();
    if (v.IsNearlyZero()) return;

    // 카메라의 수평 기준 전/우 벡터
    const float dt = GetWorld()->GetDeltaSeconds();
    const FRotator YawOnly(0.f, GetActorRotation().Yaw, 0.f);
    const FVector Fwd = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

    FVector delta = (Fwd * v.Y + Right * v.X) * MoveSpeed * dt;
    AddActorWorldOffset(delta, true);
    ClampToBoard();

    UpdateWidgetScaleForAllPawns();
}

void ACameraPawn::Zoom(const FInputActionValue& Value)
{
    const float wheel = Value.Get<float>();
    SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength - wheel * ZoomSpeed, MinArm, MaxArm);
    ClampToBoard();

    UpdateWidgetScaleForAllPawns();
}

void ACameraPawn::Pan(const FInputActionValue& Value)
{
    if (!bDragging) return;
    const FVector2D d = Value.Get<FVector2D>(); // 마우스 델타(픽셀)
    if (d.IsNearlyZero()) return;

    const FRotator YawOnly(0.f, GetActorRotation().Yaw, 0.f);
    const FVector Fwd = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

    // 화면에서 드래그한 방향대로 보드 평면을 Shift
    FVector delta = (-Right * d.X + -Fwd * d.Y) * PanSpeed;
    AddActorWorldOffset(delta, true);
    ClampToBoard();

    UpdateWidgetScaleForAllPawns();
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

