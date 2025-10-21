#pragma once
#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "SkillTypes.generated.h"

UENUM(BlueprintType)
enum class ESkillTargetMode : uint8
{
    Self,
    Unit,
    Tile,
    Line,
    Cone,
    GlobalNone
};

UENUM(BlueprintType)
enum class ETeamFilter : uint8
{
    Enemy,
    Ally,
    Any
};

USTRUCT(BlueprintType)
struct FSkillEffect
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bDealsDamage = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bDealsDamage"))
    float DamageBase = 0.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bDealsDamage"))
    float ArmorIgnore = 0.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float BuffDurationTurns = 0.f;

    // TODO: 넉백/스턴/힐/AP변경/장판 등 필요 시 확장
};

// 스킬별 사거리 계산 메트릭
UENUM(BlueprintType)
enum class EGridDistanceMetric : uint8
{
    Chebyshev UMETA(DisplayName = "Chebyshev"),
    Manhattan UMETA(DisplayName = "Manhattan")
};

// ===================== VFX / 투사체 추가 정의 =====================

// (선택) 내부적으로 슬롯 구분이 필요할 때 사용: 캐스트/임팩트
UENUM(BlueprintType)
enum class ESimpleVfxSlot : uint8
{
    Cast,
    Impact
};

// 원샷/오라 등 간단 VFX 스펙 (루프 지원)
USTRUCT(BlueprintType)
struct FSimpleVfxSpec
{
    GENERATED_BODY()

    // 재생할 나이아가라 시스템 (없으면 사운드만 재생 가능)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<UNiagaraSystem> Niagara = nullptr;

    // 동시 재생할 사운드 (선택)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<USoundBase> Sound = nullptr;

    // 루프 여부 (true면 시작/정지로 관리, false면 원샷)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bLoop = false;

    // 부착/소켓 옵션
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bAttach = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName SocketName = NAME_None;

    // 위치/회전 오프셋
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FVector LocationOffset = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FRotator RotationOffset = FRotator::ZeroRotator;

    // 루프 정지 시 약간의 꼬리를 남길 딜레이(초). 0이면 즉시 Destroy.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float LoopStopDelay = 0.2f;
};

// 투사체 이동 방식
UENUM(BlueprintType)
enum class EProjectileMotion : uint8
{
    Instant,   // 즉발(원샷)
    Linear,    // 직사체
    Ballistic  // 곡사체(포물선)
};

// 코스메틱 전용 투사체 스펙 (충돌/요격 없이 연출과 지연만 제공)
USTRUCT(BlueprintType)
struct FSimpleProjectileSpec
{
    GENERATED_BODY()

    // true면 Travel 단계 사용(아니면 즉발 흐름)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bUseProjectile = false;

    // 이동 방식(Instant/Linear/Ballistic)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    EProjectileMotion Motion = EProjectileMotion::Linear;

    // 공통: 속도(uu/sec)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile", ClampMin = "1.0"))
    float Speed = 1200.f;

    // 이 거리 이하이면 즉발로 스냅(원샷 손맛 유지)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile", ClampMin = "0.0"))
    float ShortAsInstantDistance = 150.f;

    // 이동 시간 클램프(너무 짧거나 길지 않게)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile", ClampMin = "0.0"))
    float MinTravelTime = 0.06f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile", ClampMin = "0.0"))
    float MaxTravelTime = 1.25f;

    // 곡사 최고점 높이(uu). Motion=Ballistic일 때만 의미.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    float ArcHeight = 300.f;

    // 비행(Travel) 중 보여줄 나이아가라
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    TObjectPtr<UNiagaraSystem> TravelNiagara = nullptr;

    // 시작점(소스) 부착 옵션
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    bool bAttachToSource = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    FName SourceSocket = NAME_None;
};