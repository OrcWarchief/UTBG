// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Tile/TileType.h"
#include "GamePlay/Team/Team.h"
#include "GenericTeamAgentInterface.h"
#include "PawnBase.generated.h"

class ABoard;
class UStaticMeshComponent;
class AUTBGPlayerState;
class UWidgetComponent;
class UPawnWidget;
class UAnimMontage;
class UUnitSkillsComponent;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPawnDied, class APawnBase*, Pawn);

UCLASS()
class UTBG_API APawnBase : public APawn, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	APawnBase();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetTeamColor(ETeam TeamToSet);
    void UpdateHealthWidget();
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable) int32 GetCurrentHP() const { return CurrentHP; }
    UFUNCTION(BlueprintCallable) int32 GetMaxHP() const { return MaxHP; }

    UFUNCTION(BlueprintCallable, Category = "Attributes|Shield")
    void SetMaxShield(float NewMax, bool bClampCurrent = true);

    UFUNCTION(BlueprintCallable, Category = "Attributes|Shield")
    void AddShield(float Amount);
    
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetOnTile(ATileActor* NewTile);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetGridCoord(FIntPoint NewCoord);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlaySkillMontage(UAnimMontage* Montage, FName Section = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Appearance")
    class UMeshComponent* GetVisualComponent() const;
    
    UFUNCTION() void OnRep_CurrentHP();
    UFUNCTION() void OnRep_GridCoord(FIntPoint PrevCoord);
    UFUNCTION() void OnRep_Shield();
    UFUNCTION() void OnRep_Board();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UStaticMeshComponent* Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USkeletalMeshComponent* Avatar;

    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    TSubclassOf<class UAnimInstance> DefaultAnimClass;

    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    TArray<class UMaterialInterface*> DefaultMaterials;

    UPROPERTY(ReplicatedUsing = OnRep_GridCoord, EditInstanceOnly, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "-1"))
    FIntPoint GridCoord = FIntPoint(-1, -1);    // 에디터에서 인스턴스별로 직접 X,Y를 설정 -1은 미 지정 값

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    FVector PawnOffset = FVector::ZeroVector;

    UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadWrite, Category = "Board")
    ABoard* Board = nullptr;

    // 현재 서 있는 타일
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    ATileActor* CurrentTile = nullptr;

    // Unit Stats
    // 팀 정보는 PlayerState가 권위가지고 있고 Pawn의 TeamId는 캐시
    UPROPERTY(EditAnywhere, Category = "Team")
    ETeam Team = ETeam::ET_NoTeam;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MaxHP = 10;

    UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentHP, Category = "Unit")
    int32 CurrentHP = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 Damage = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 AttackCost = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 AttackRange = 1;   // 맨해튼 거리

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    int32 MoveRange = 1;

    UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Unit", meta = (ClampMin = "0.0"))
    float MaxShield = 0.f;

    UPROPERTY(ReplicatedUsing = OnRep_Shield, BlueprintReadOnly, Category = "Unit", meta = (ClampMin = "0.0"))
    float Shield = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UUnitSkillsComponent* Skills;

    /**
    * Team colors
    */
    UPROPERTY(VisibleAnywhere, Category = Elim)
    UMaterialInstance* DissolveMaterialInstance;

    UPROPERTY(EditAnywhere, Category = Elim)
    UMaterialInstance* RedDissolveMatInst;

    UPROPERTY(EditAnywhere, Category = Elim)
    UMaterialInstance* RedMaterial;

    UPROPERTY(EditAnywhere, Category = Elim)
    UMaterialInstance* BlueDissolveMatInst;

    UPROPERTY(EditAnywhere, Category = Elim)
    UMaterialInstance* BlueMaterial;

    UPROPERTY(EditAnywhere, Category = Elim)
    UMaterialInstance* OriginalMaterial;

    UPROPERTY()
    AUTBGPlayerState* UTBGPlayerState;

    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Unit")
    bool bIsKing = false;

    /** 사망 시 스폰할 나이아가라 이펙트(선택). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|VFX")
    UNiagaraSystem* DeathVFX = nullptr;

    /** 사망 시 재생할 애님 몽타주(선택, 스켈레탈 메쉬가 있을 때만). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Anim")
    UAnimMontage* DeathMontage = nullptr;

    /** 연출 최소 지연(몽타주 길이보다 짧으면 이 값으로 보정). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
    float DeathDelay = 2.0f;

    /** (추가) 몽타주 재생 속도 및 여유 지연 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Anim")
    float DeathMontagePlayRate = 1.0f;

    /** 서버 전용: 사망 판정 후 한 번만 호출. */
    UFUNCTION(BlueprintCallable, Category = "Death")
    void HandleDeath(AActor* DamageCauser);

    /** 모든 클라이언트에서 VFX/애님을 재생. */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastDeathEffects();

    /** GameMode/GameState가 구독할 수 있는 사망 이벤트(서버 브로드캐스트). */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPawnDied OnPawnDied;

    // ---- Attack (추가) ----
    /** 공격 몽타주 (DefaultSlot) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Anim")
    UAnimMontage* AttackMontage = nullptr;

    /** 공격 몽타주 재생 속도 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Anim")
    float AttackMontagePlayRate = 1.0f;

    /** 공격 몽타주 재생 (아무 곳에서 호출 가능. 서버면 즉시 멀티캐스트, 클라면 서버 RPC) */
    UFUNCTION(BlueprintCallable, Category = "Combat|Anim")
    void PlayAttackMontage(FName Section = NAME_None);

    /** 클라이언트 → 서버: 공격 몽타주 재생 요청 */
    UFUNCTION(Server, Reliable)
    void Server_PlayAttackMontage(FName Section);
protected:
	virtual void BeginPlay() override;
    virtual void Destroyed() override;

private:
    // 위젯
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UWidgetComponent* PawnWidgetComp;

    UPROPERTY()
    UPawnWidget* PawnWidget;

    bool bDying = false;

public:	
    UFUNCTION(BlueprintPure, Category = "Grid")
    FORCEINLINE FIntPoint GetGridCoord() const { return GridCoord; }

    UFUNCTION(BlueprintPure, Category = "Combat")
    FORCEINLINE bool IsDead() const { return CurrentHP <= 0; }
};
