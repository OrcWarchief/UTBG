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
    FIntPoint GridCoord = FIntPoint(-1, -1);    // �����Ϳ��� �ν��Ͻ����� ���� X,Y�� ���� -1�� �� ���� ��

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    FVector PawnOffset = FVector::ZeroVector;

    UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadWrite, Category = "Board")
    ABoard* Board = nullptr;

    // ���� �� �ִ� Ÿ��
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    ATileActor* CurrentTile = nullptr;

    // Unit Stats
    // �� ������ PlayerState�� ���������� �ְ� Pawn�� TeamId�� ĳ��
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
    int32 AttackRange = 1;   // ����ư �Ÿ�

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

    /** ��� �� ������ ���̾ư��� ����Ʈ(����). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|VFX")
    UNiagaraSystem* DeathVFX = nullptr;

    /** ��� �� ����� �ִ� ��Ÿ��(����, ���̷�Ż �޽��� ���� ����). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Anim")
    UAnimMontage* DeathMontage = nullptr;

    /** ���� �ּ� ����(��Ÿ�� ���̺��� ª���� �� ������ ����). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
    float DeathDelay = 2.0f;

    /** (�߰�) ��Ÿ�� ��� �ӵ� �� ���� ���� */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Anim")
    float DeathMontagePlayRate = 1.0f;

    /** ���� ����: ��� ���� �� �� ���� ȣ��. */
    UFUNCTION(BlueprintCallable, Category = "Death")
    void HandleDeath(AActor* DamageCauser);

    /** ��� Ŭ���̾�Ʈ���� VFX/�ִ��� ���. */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastDeathEffects();

    /** GameMode/GameState�� ������ �� �ִ� ��� �̺�Ʈ(���� ��ε�ĳ��Ʈ). */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPawnDied OnPawnDied;

    // ---- Attack (�߰�) ----
    /** ���� ��Ÿ�� (DefaultSlot) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Anim")
    UAnimMontage* AttackMontage = nullptr;

    /** ���� ��Ÿ�� ��� �ӵ� */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Anim")
    float AttackMontagePlayRate = 1.0f;

    /** ���� ��Ÿ�� ��� (�ƹ� ������ ȣ�� ����. ������ ��� ��Ƽĳ��Ʈ, Ŭ��� ���� RPC) */
    UFUNCTION(BlueprintCallable, Category = "Combat|Anim")
    void PlayAttackMontage(FName Section = NAME_None);

    /** Ŭ���̾�Ʈ �� ����: ���� ��Ÿ�� ��� ��û */
    UFUNCTION(Server, Reliable)
    void Server_PlayAttackMontage(FName Section);
protected:
	virtual void BeginPlay() override;
    virtual void Destroyed() override;

private:
    // ����
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
