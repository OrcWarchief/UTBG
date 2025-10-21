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

    // TODO: �˹�/����/��/AP����/���� �� �ʿ� �� Ȯ��
};

// ��ų�� ��Ÿ� ��� ��Ʈ��
UENUM(BlueprintType)
enum class EGridDistanceMetric : uint8
{
    Chebyshev UMETA(DisplayName = "Chebyshev"),
    Manhattan UMETA(DisplayName = "Manhattan")
};

// ===================== VFX / ����ü �߰� ���� =====================

// (����) ���������� ���� ������ �ʿ��� �� ���: ĳ��Ʈ/����Ʈ
UENUM(BlueprintType)
enum class ESimpleVfxSlot : uint8
{
    Cast,
    Impact
};

// ����/���� �� ���� VFX ���� (���� ����)
USTRUCT(BlueprintType)
struct FSimpleVfxSpec
{
    GENERATED_BODY()

    // ����� ���̾ư��� �ý��� (������ ���常 ��� ����)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<UNiagaraSystem> Niagara = nullptr;

    // ���� ����� ���� (����)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<USoundBase> Sound = nullptr;

    // ���� ���� (true�� ����/������ ����, false�� ����)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bLoop = false;

    // ����/���� �ɼ�
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bAttach = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName SocketName = NAME_None;

    // ��ġ/ȸ�� ������
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FVector LocationOffset = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FRotator RotationOffset = FRotator::ZeroRotator;

    // ���� ���� �� �ణ�� ������ ���� ������(��). 0�̸� ��� Destroy.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float LoopStopDelay = 0.2f;
};

// ����ü �̵� ���
UENUM(BlueprintType)
enum class EProjectileMotion : uint8
{
    Instant,   // ���(����)
    Linear,    // ����ü
    Ballistic  // ���ü(������)
};

// �ڽ���ƽ ���� ����ü ���� (�浹/��� ���� ����� ������ ����)
USTRUCT(BlueprintType)
struct FSimpleProjectileSpec
{
    GENERATED_BODY()

    // true�� Travel �ܰ� ���(�ƴϸ� ��� �帧)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bUseProjectile = false;

    // �̵� ���(Instant/Linear/Ballistic)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    EProjectileMotion Motion = EProjectileMotion::Linear;

    // ����: �ӵ�(uu/sec)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile", ClampMin = "1.0"))
    float Speed = 1200.f;

    // �� �Ÿ� �����̸� ��߷� ����(���� �ո� ����)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile", ClampMin = "0.0"))
    float ShortAsInstantDistance = 150.f;

    // �̵� �ð� Ŭ����(�ʹ� ª�ų� ���� �ʰ�)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile", ClampMin = "0.0"))
    float MinTravelTime = 0.06f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile", ClampMin = "0.0"))
    float MaxTravelTime = 1.25f;

    // ��� �ְ��� ����(uu). Motion=Ballistic�� ���� �ǹ�.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    float ArcHeight = 300.f;

    // ����(Travel) �� ������ ���̾ư���
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    TObjectPtr<UNiagaraSystem> TravelNiagara = nullptr;

    // ������(�ҽ�) ���� �ɼ�
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    bool bAttachToSource = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bUseProjectile"))
    FName SourceSocket = NAME_None;
};