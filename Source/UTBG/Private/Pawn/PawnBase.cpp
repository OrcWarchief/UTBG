// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/PawnBase.h"
#include "Board/Board.h"
#include "Tile/TileActor.h"
#include "HUD/PawnWidget.h"
#include "Data/SkillData.h"
#include "GameState/UTBGGameState.h"
#include "GamePlay/Team/TeamUtils.h"
#include "UTBGComponents/UnitSkillsComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h" 
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

APawnBase::APawnBase()
{
	PrimaryActorTick.bCanEverTick = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	Avatar = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Avatar"));
	Avatar->SetupAttachment(RootComponent);
	Avatar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Avatar->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	Avatar->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	bReplicates = true;              // 이 액터를 네트워크로 복제
	bAlwaysRelevant = true;			 // 전역 시야
	SetReplicateMovement(true);		 // 위치/회전/스케일 등 이동 복제

	AutoPossessPlayer = EAutoReceiveInput::Disabled; // 카메라 폰이 따로 있으니 소유 불필요
	AutoPossessAI = EAutoPossessAI::Disabled;        // AI는 나중에 붙일 예정

	PawnWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("PawnWidget"));
	PawnWidgetComp->SetupAttachment(RootComponent);

	Skills = CreateDefaultSubobject<UUnitSkillsComponent>(TEXT("Skills"));
	Skills->SetIsReplicated(true);
}

UMeshComponent* APawnBase::GetVisualComponent() const
{
	if (Avatar && Avatar->GetSkeletalMeshAsset())
	{
		return Avatar;
	}
	return Body;
}

void APawnBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APawnBase, CurrentHP);
	DOREPLIFETIME(APawnBase, GridCoord);
	DOREPLIFETIME(APawnBase, MaxShield);
	DOREPLIFETIME(APawnBase, Shield);
	DOREPLIFETIME(APawnBase, Board);
	DOREPLIFETIME(APawnBase, bIsKing);
}

void APawnBase::SetTeamColor(ETeam TeamToSet)
{
	UMeshComponent* Visual = GetVisualComponent();
	if (!IsValid(Visual) || OriginalMaterial == nullptr) return;

	UMaterialInterface* Mat = OriginalMaterial;
	UMaterialInstance* Dissolve = BlueDissolveMatInst;

	switch (TeamToSet)
	{
	case ETeam::ET_BlueTeam: 
		Mat = BlueMaterial;  
		Dissolve = BlueDissolveMatInst; 
		break;
	case ETeam::ET_RedTeam:  
		Mat = RedMaterial;   
		Dissolve = RedDissolveMatInst;  
		break;
	case ETeam::ET_NoTeam:

	default:                 
		Mat = OriginalMaterial; 
		Dissolve = BlueDissolveMatInst; 
		break;
	}

	for (int32 i = 0; i < Visual->GetNumMaterials(); ++i)
	{
		Visual->SetMaterial(i, Mat);
	}
	DissolveMaterialInstance = Dissolve;
}

void APawnBase::UpdateHealthWidget()
{
	if (!PawnWidget && PawnWidgetComp)
	{
		if (UUserWidget* W = PawnWidgetComp->GetUserWidgetObject())
		{
			PawnWidget = Cast<UPawnWidget>(W);
		}
	}

	if (PawnWidget)
	{
		const int32 Cur = CurrentHP, Max = MaxHP;
		PawnWidget->SetHealth(Cur, Max);
		UE_LOG(LogTemp, Warning, TEXT("[UpdateHealthWidget] %s -> %d/%d"), *GetName(), Cur, Max);
	}
}

void APawnBase::UpdateShieldWidget()
{
	if (!PawnWidget && PawnWidgetComp)
	{
		if (UUserWidget* W = PawnWidgetComp->GetUserWidgetObject())
		{
			PawnWidget = Cast<UPawnWidget>(W);
		}
	}
	if (PawnWidget)
	{
		PawnWidget->SetShield(Shield, MaxShield);
		UE_LOG(LogTemp, Verbose, TEXT("[UpdateShieldWidget] %s -> %.0f/%.0f"), *GetName(), Shield, MaxShield);
	}
}

void APawnBase::SetOnTile(ATileActor* NewTile)
{
	if (NewTile == nullptr) return;
	CurrentTile = NewTile;
	SetActorLocation(CurrentTile->GetActorLocation());
}

void APawnBase::HandleHPChanged()
{
	UpdateHealthWidget();
	UE_LOG(LogTemp, Warning, TEXT("[OnRep_CurrentHP] %s: %d / %d"), *GetName(), CurrentHP, MaxHP);
	OnHPChanged.Broadcast(this, CurrentHP);
}

void APawnBase::HandleShieldChanged()
{
	UpdateShieldWidget();
	UE_LOG(LogTemp, Warning, TEXT("[OnRep_CurrentShield] %s: %f / %f"), *GetName(), Shield, MaxShield);
	OnShieldChanged.Broadcast(this, Shield);
}

void APawnBase::OnRep_CurrentHP()
{
	HandleHPChanged();
}

void APawnBase::OnRep_Shield()
{
	HandleShieldChanged();
}
void APawnBase::OnRep_GridCoord(FIntPoint PrevCoord)
{
	if (Board && GridCoord.X >= 0 && GridCoord.Y >= 0)
	{
		Board->UpdatePawnCell(this, PrevCoord, GridCoord);
		SetActorLocation(Board->GridToWorld(GridCoord) + PawnOffset);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[OnRep_GridCoord] %s: Board is null or invalid coord (%d,%d). Will snap when Board arrives."),
			*GetName(), GridCoord.X, GridCoord.Y);
	}
}

void APawnBase::OnRep_Board()
{
	if (!Board) return;

	if (GridCoord.X >= 0 && GridCoord.Y >= 0)
	{
		SetActorLocation(Board->GridToWorld(GridCoord) + PawnOffset);
	}

	Board->RegisterPawn(this);
}

void APawnBase::SetGridCoord(FIntPoint NewCoord)
{
	const FIntPoint Prev = GridCoord;
	GridCoord = NewCoord;

	if (Board && GridCoord.X >= 0 && GridCoord.Y >= 0)
	{
		Board->UpdatePawnCell(this, Prev, NewCoord);
		SetActorLocation(Board->GridToWorld(GridCoord) + PawnOffset);
	}

	ForceNetUpdate();
}

void APawnBase::StartSkill_NotifyDriven(USkillData* InData, AActor* InTarget)
{
	if (!InData) return;

	if (!HasAuthority())
	{
		Server_StartSkill_NotifyDriven(InData, InTarget);
		return;
	}

	if (AUTBGGameState* GS = GetWorld() ? GetWorld()->GetGameState<AUTBGGameState>() : nullptr)
	{
		const ETeam MyTeam = UTeamUtils::GetActorTeam(this);
		if (!GS->IsTeamTurn(MyTeam))
		{
			UE_LOG(LogTemp, Verbose, TEXT("[Skill] BLOCK: Not your turn"));
			return;
		}
		if (GS->bResolving)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[Skill] BLOCK: Resolving in progress"));
			return;
		}
		if (!GS->HasAPForActor(this, InData->APCost))
		{
			UE_LOG(LogTemp, Verbose, TEXT("[Skill] BLOCK: Not enough AP"));
			return;
		}
		GS->SetResolving(true);
	}

	// 서버: 이번 캐스팅 컨텍스트 저장
	CurrentSkillData = InData;
	CurrentSkillTarget = (InTarget != nullptr) ? InTarget : this;

	bool bUsingMontage = false;
	if (InData->CastMontage && Avatar && GetNetMode() != NM_DedicatedServer)
	{
		if (UAnimInstance* Anim = Avatar->GetAnimInstance())
		{
			bUsingMontage = true;
		}
	}


	if (bUsingMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Skill] Start with MONTAGE: %s Section=%s"),
			*GetNameSafe(InData->CastMontage), *InData->CastSection.ToString());
		MulticastPlaySkillMontage(InData->CastMontage, InData->CastSection);

		const float ImpactDelay = FMath::Max(0.f, InData->HitTimeSec);
		GetWorldTimerManager().SetTimer(
			ImpactFallbackHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					if (!HasAuthority()) return;
					if (UUnitSkillsComponent* Comp = FindComponentByClass<UUnitSkillsComponent>())
					{
						if (CurrentSkillData)
						{
							AActor* T = CurrentSkillTarget.IsValid() ? CurrentSkillTarget.Get() : this;
							Comp->PlayProjectileOrInstant(CurrentSkillData, T);
						}
					}
				}),
			ImpactDelay + 0.05f, false);

		float Dur = 0.f;
		if (InData->CastMontage)
		{
			const int32 SecIdx = InData->CastMontage->GetSectionIndex(InData->CastSection);
			Dur = (SecIdx != INDEX_NONE) ? InData->CastMontage->GetSectionLength(SecIdx) : InData->CastMontage->GetPlayLength();
		}
		if (Dur <= 0.f) Dur = ImpactDelay + 0.2f;

		GetWorldTimerManager().SetTimer(
			CastEndFallbackHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					if (!HasAuthority()) return;
					if (UUnitSkillsComponent* Comp = FindComponentByClass<UUnitSkillsComponent>())
					{
						if (CurrentSkillData) Comp->StopCastVfx(CurrentSkillData);
					}
					ClearSkillContext();
				}),
			Dur + 0.1f, false);

		return;
	}

	if (UUnitSkillsComponent* Comp = Skills)
	{
		Comp->PlayCastVfx(InData);

		const float Delay = FMath::Max(0.f, InData->HitTimeSec);

		GetWorldTimerManager().SetTimer(
			ImpactFallbackHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
					if (!HasAuthority()) return;
					if (!Skills || !CurrentSkillData) return;

					AActor* TargetA = CurrentSkillTarget.IsValid() ? CurrentSkillTarget.Get() : this;
					Skills->PlayProjectileOrInstant(CurrentSkillData, TargetA); // 내부에서 비용/효과/Resolving
			}),
			Delay, false);

		GetWorldTimerManager().SetTimer(
			CastEndFallbackHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					if (!HasAuthority()) return;
					if (UUnitSkillsComponent* Comp2 = FindComponentByClass<UUnitSkillsComponent>())
					{
						if (CurrentSkillData) Comp2->StopCastVfx(CurrentSkillData);
					}
					ClearSkillContext();
				}),
			Delay + 0.1f, false);
	}
}

void APawnBase::Server_StartSkill_NotifyDriven_Implementation(USkillData* InData, AActor* InTarget)
{
	StartSkill_NotifyDriven(InData, InTarget);
}

void APawnBase::ClearSkillContext()
{
	// 폴백 타이머 정리
	GetWorldTimerManager().ClearTimer(ImpactFallbackHandle);
	GetWorldTimerManager().ClearTimer(CastEndFallbackHandle);

	CurrentSkillData = nullptr;
	CurrentSkillTarget = nullptr;
}

void APawnBase::BP_OnCastStartNotify()
{
	if (!HasAuthority()) return;
	if (UUnitSkillsComponent* Comp = FindComponentByClass<UUnitSkillsComponent>())
	{
		if (CurrentSkillData) { Comp->PlayCastVfx(CurrentSkillData); }
	}
}

void APawnBase::BP_OnSkillImpactNotify()
{
	if (!HasAuthority()) return;
	GetWorldTimerManager().ClearTimer(ImpactFallbackHandle);

	if (UUnitSkillsComponent* Comp = FindComponentByClass<UUnitSkillsComponent>())
	{
		if (CurrentSkillData)
		{
			AActor* T = CurrentSkillTarget.IsValid() ? CurrentSkillTarget.Get() : this;
			Comp->PlayProjectileOrInstant(CurrentSkillData, T);
		}
	}
}

void APawnBase::BP_OnCastEndNotify()
{
	if (!HasAuthority()) return;
	GetWorldTimerManager().ClearTimer(CastEndFallbackHandle);

	if (UUnitSkillsComponent* Comp = FindComponentByClass<UUnitSkillsComponent>())
	{
		if (CurrentSkillData) { Comp->StopCastVfx(CurrentSkillData); }
	}
	ClearSkillContext();
}

void APawnBase::MulticastPlaySkillMontage_Implementation(UAnimMontage* Montage, FName Section)
{
	if (!Montage || !Avatar) return;

	if (UAnimInstance* Anim = Avatar->GetAnimInstance())
	{
		float PlayRate = 1.f;
		if		(	Montage == DeathMontage		)	{ PlayRate = DeathMontagePlayRate;	}
		else if (	Montage == AttackMontage	)	{ PlayRate = AttackMontagePlayRate; }

		const float Len = Anim->Montage_Play(Montage, PlayRate);
		if (Len <= 0.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Montage] Play failed: %s"), *GetNameSafe(Montage));
			return;
		}

		if (Section != NAME_None)
		{
			const int32 SecIdx = Montage->GetSectionIndex(Section);
			if (SecIdx != INDEX_NONE)
			{
				Anim->Montage_JumpToSection(Section, Montage);
			}
		}
	}
}

float APawnBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority()) return 0.f;

	const float SuperAppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	const float RawDamage = FMath::Max(SuperAppliedDamage, DamageAmount); // 보수적으로 더 큰 값을 채택
	float Remaining = FMath::Max(0.f, RawDamage);

	if (Shield > 0.f && Remaining > 0.f)
	{
		const float Absorb = FMath::Min(Shield, Remaining);
		Shield = FMath::Max(0.f, Shield - Absorb);
		Remaining -= Absorb;

		HandleShieldChanged();
		ForceNetUpdate();
	}
	
	const int32 HpDamage = FMath::Max(0, FMath::RoundToInt(Remaining));
	if (HpDamage <= 0 || IsDead()) { UE_LOG(LogTemp, Verbose, TEXT("%s with Shield (Shield: %.0f/%.0f)"), *GetName(), Shield, MaxShield); return 0.f; }
	// HP 감소
	const int32 NewHP = FMath::Clamp(CurrentHP - HpDamage, 0, MaxHP);
	
	UE_LOG(LogTemp, Verbose, TEXT("%s took %d HP damage (HP: %d -> %d, Shield: %.0f/%.0f)"),
		*GetName(), HpDamage,  CurrentHP, NewHP, Shield, MaxShield);
	// 죽음 처리
	if (NewHP != CurrentHP)
	{
		CurrentHP = NewHP;

		HandleHPChanged();
		if (IsDead())
		{
			HandleDeath(DamageCauser);
		}
		ForceNetUpdate();
	}
	return HpDamage;
}

void APawnBase::ApplyWidgetScale(float NewScale)
{
	if (GetNetMode() == NM_DedicatedServer) return;
	if (!PawnWidgetComp) return;

	NewScale = FMath::Clamp(NewScale, 0.3f, 1.25f);
	if (FMath::IsNearlyEqual(LastAppliedWidgetScale, NewScale, 0.01f)) return;

	// Screenspace
	if (PawnWidgetComp->GetWidgetSpace() == EWidgetSpace::Screen)
	{
		if (!PawnWidget)
			PawnWidget = Cast<UPawnWidget>(PawnWidgetComp->GetUserWidgetObject());
		if (PawnWidget)
		{
			PawnWidget->SetRenderScale(FVector2D(NewScale, NewScale));
		}
	}
	else
	{
		// Fallback
		PawnWidgetComp->SetWorldScale3D(FVector(NewScale));
	}

	LastAppliedWidgetScale = NewScale;
	UE_LOG(LogTemp, Log, TEXT("%s RenderScale=%.2f"), *GetName(), NewScale);
}

void APawnBase::ApplyWidgetLOD(int32 LOD)
{
	if (!PawnWidget && PawnWidgetComp)
	{
		PawnWidget = Cast<UPawnWidget>(PawnWidgetComp->GetUserWidgetObject());
	}
	if (PawnWidget)
	{
		PawnWidget->SetCompactModeByLOD(LOD); // 0=bar only, 1=hp text, 2=full
	}
}

void APawnBase::SetMaxShield(float NewMax, bool bClampCurrent)
{
	if (!HasAuthority()) return;
	MaxShield = FMath::Max(0.f, NewMax);
	if (bClampCurrent) 
	{ 
		Shield = FMath::Clamp(Shield, 0.f, MaxShield); 
	}

	HandleShieldChanged();
	ForceNetUpdate();
}

void APawnBase::AddShield(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f) return;
	Shield = FMath::Clamp(Shield + Amount, 0.f, MaxShield);
	
	HandleShieldChanged();
	ForceNetUpdate();
}

void APawnBase::HandleDeath(AActor* DamageCauser)
{
	if (!HasAuthority() || bDying) return;
	bDying = true;

	SetActorEnableCollision(false);
	OnPawnDied.Broadcast(this);

	MulticastDeathEffects();

	if (DeathMontage && Avatar)
	{
		if (UAnimInstance* Anim = Avatar->GetAnimInstance())
		{
			const float PlayRate = (DeathMontagePlayRate > 0.f) ? DeathMontagePlayRate : 1.f;
			Anim->Montage_Play(DeathMontage, PlayRate);
		}
	}

	SetLifeSpan(DeathDelay);
}

void APawnBase::MulticastDeathEffects_Implementation()
{
	// 전용서버는 연출 필요 없음
	if (GetNetMode() == NM_DedicatedServer) return;

	// Visual 전체 슬롯에 디졸브 적용
	if (UMeshComponent* Visual = GetVisualComponent())
	{
		if (DissolveMaterialInstance)
		{
			for (int32 i = 0; i < Visual->GetNumMaterials(); ++i)
			{
				Visual->SetMaterial(i, DissolveMaterialInstance);
			}
		}
	}

	if (DeathVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DeathVFX, GetActorLocation(), GetActorRotation());
	}
}

void APawnBase::PlayAttackMontage(FName Section)
{
}

void APawnBase::Server_PlayAttackMontage_Implementation(FName Section)
{
}

void APawnBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = FMath::Clamp(CurrentHP, 0, MaxHP);

	if (Board)
	{
		Board->RegisterPawn(this);

		// 시작 좌표가 설정되어 있으면 즉시 스냅
		if (GridCoord.X >= 0 && GridCoord.Y >= 0)
		{
			SetActorLocation(Board->GridToWorld(GridCoord) + PawnOffset);
		}
	}

	if (PawnWidgetComp)
	{
		UUserWidget* Widget = PawnWidgetComp->GetUserWidgetObject();
		PawnWidget = Cast<UPawnWidget>(Widget);
		PawnWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
		PawnWidgetComp->SetDrawAtDesiredSize(true);
		PawnWidgetComp->SetPivot(FVector2D(0.5f, 1.0f));
		PawnWidgetComp->SetTwoSided(true);
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		HandleHPChanged();
		HandleShieldChanged();
	}
}

void APawnBase::Destroyed() {
	if (Board) Board->UnRegisterPawn(this);
	Super::Destroyed();
}
