#pragma once
#include "CoreMinimal.h"
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