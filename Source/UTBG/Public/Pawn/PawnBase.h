// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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
class USkillData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPawnDied, class APawnBase*, Pawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, class APawnBase*, Pawn, int32, NewHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldChanged, class APawnBase*, Pawn, float, NewShield);

UCLASS()
class UTBG_API APawnBase : public APawn, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	APawnBase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Widget
	UFUNCTION(BlueprintCallable, Category = "UI|Widget")
	void ApplyWidgetScale(float NewScale);               // Screen Space -> RenderScale
	// Camera/LOD system calls this (0: far, 1: mid, 2: near)
	UFUNCTION(BlueprintCallable, Category = "UI|Widget")
	void ApplyWidgetLOD(int32 LOD);

	void SetTeamColor(ETeam TeamToSet);
	void UpdateHealthWidget();
	void UpdateShieldWidget();

	// ----- Attributes / Combat -----
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	UFUNCTION(BlueprintCallable) int32 GetCurrentHP() const { return CurrentHP; }
	UFUNCTION(BlueprintCallable) int32 GetMaxHP()   const { return MaxHP; }

	UFUNCTION(BlueprintCallable, Category = "Attributes|Shield")
	void SetMaxShield(float NewMax, bool bClampCurrent = true);
	UFUNCTION(BlueprintCallable, Category = "Attributes|Shield")
	void AddShield(float Amount);

	// ----- Grid / Board -----
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetOnTile(class ATileActor* NewTile);
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void SetGridCoord(FIntPoint NewCoord);

	// ----- Skills / Anim bridge -----
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlaySkillMontage(UAnimMontage* Montage, FName Section = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void StartSkill_NotifyDriven(class USkillData* InData, AActor* InTarget);

	UFUNCTION(Server, Reliable)
	void Server_StartSkill_NotifyDriven(class USkillData* InData, AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void ClearSkillContext();

	UFUNCTION(BlueprintPure, Category = "Skills")
	class USkillData* GetCurrentSkillData() const { return CurrentSkillData; }

	UFUNCTION(BlueprintPure, Category = "Skills")
	AActor* GetCurrentSkillTarget() const { return CurrentSkillTarget.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Skills|Bridge")
	void BP_OnCastStartNotify();

	UFUNCTION(BlueprintCallable, Category = "Skills|Bridge")
	void BP_OnSkillImpactNotify();

	UFUNCTION(BlueprintCallable, Category = "Skills|Bridge")
	void BP_OnCastEndNotify();

	UFUNCTION(BlueprintCallable, Category = "Appearance")
	class UMeshComponent* GetVisualComponent() const;

	UFUNCTION() void OnRep_CurrentHP();
	UFUNCTION() void OnRep_GridCoord(FIntPoint PrevCoord);
	UFUNCTION() void OnRep_Shield();
	UFUNCTION() void OnRep_Board();

	void HandleHPChanged();
	void HandleShieldChanged();

	// ----- Components / Data -----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* Avatar;

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	TSubclassOf<class UAnimInstance> DefaultAnimClass;

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	TArray<class UMaterialInterface*> DefaultMaterials;

	UPROPERTY(ReplicatedUsing = OnRep_GridCoord, EditInstanceOnly, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "-1"))
	FIntPoint GridCoord = FIntPoint(-1, -1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FVector PawnOffset = FVector::ZeroVector;

	UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadWrite, Category = "Board")
	ABoard* Board = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	class ATileActor* CurrentTile = nullptr;

	// Unit Stats
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
	int32 AttackRange = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit") 
	int32 MoveRange = 1;

	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Unit", meta = (ClampMin = "0.0"))
	float MaxShield = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, BlueprintReadOnly, Category = "Unit", meta = (ClampMin = "0.0"))
	float Shield = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UUnitSkillsComponent* Skills;

	UPROPERTY(BlueprintAssignable, Category = "Events|Attributes")
	FOnHPChanged OnHPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|Attributes")
	FOnShieldChanged OnShieldChanged;

	// Team colors / dissolve
	UPROPERTY(VisibleAnywhere, Category = Elim)
	class UMaterialInstance* DissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = Elim)
	class UMaterialInstance* RedDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Elim)
	class UMaterialInstance* RedMaterial;

	UPROPERTY(EditAnywhere, Category = Elim)
	class UMaterialInstance* BlueDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Elim)
	class UMaterialInstance* BlueMaterial;

	UPROPERTY(EditAnywhere, Category = Elim)
	class UMaterialInstance* OriginalMaterial;

	UPROPERTY()
	class AUTBGPlayerState* UTBGPlayerState;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Unit")
	bool bIsKing = false;

	// Death VFX / anim
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|VFX")
	UNiagaraSystem* DeathVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Anim")
	UAnimMontage* DeathMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	float DeathDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Anim")
	float DeathMontagePlayRate = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Death")
	void HandleDeath(AActor* DamageCauser);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeathEffects();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPawnDied OnPawnDied;

	// Attack montage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Anim")
	UAnimMontage* AttackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Anim")
	float AttackMontagePlayRate = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Combat|Anim")
	void PlayAttackMontage(FName Section = NAME_None);

	UFUNCTION(Server, Reliable)
	void Server_PlayAttackMontage(FName Section);

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

private:
	// ---- UI / Widget cached ----
	float LastAppliedWidgetScale = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* PawnWidgetComp;

	UPROPERTY()
	UPawnWidget* PawnWidget;

	// Head-height offset used for placing the widget in screen space too
	UPROPERTY(EditAnywhere, Category = "UI|Widget")
	float WidgetZOffset = 20.f;

	bool bDying = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skills", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkillData> CurrentSkillData = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Skills", meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AActor> CurrentSkillTarget;

	FTimerHandle ImpactFallbackHandle;
	FTimerHandle CastEndFallbackHandle;

public:
	UFUNCTION(BlueprintPure, Category = "Grid")
	FORCEINLINE FIntPoint GetGridCoord() const { return GridCoord; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE bool IsDead() const { return CurrentHP <= 0; }
};