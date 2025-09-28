// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/PawnBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Animation/AnimInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h" 
#include "Board/Board.h"
#include "GameFramework/Controller.h"
#include "Engine/Engine.h"
#include "Tile/TileActor.h"
#include "Components/WidgetComponent.h"
#include "HUD/PawnWidget.h"
#include "Net/UnrealNetwork.h"
#include "UTBGComponents/UnitSkillsComponent.h"
#include "Animation/AnimMontage.h"

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

void APawnBase::SetOnTile(ATileActor* NewTile)
{
	if (NewTile == nullptr) return;
	CurrentTile = NewTile;
	SetActorLocation(CurrentTile->GetActorLocation());
}

void APawnBase::OnRep_CurrentHP()
{
	UE_LOG(LogTemp, Warning, TEXT("[OnRep_CurrentHP] %s: %d / %d"), *GetName(), CurrentHP, MaxHP);
	UpdateHealthWidget();
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

void APawnBase::OnRep_Shield()
{
	// TODO: UI, FX
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

	ForceNetUpdate(); // 복제 틱을 앞당겨주는 효과
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

	// Super 호출(플러그인/시스템이 Damage를 후킹한 경우 대비)
	const float SuperAppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	const float RawDamage = FMath::Max(SuperAppliedDamage, DamageAmount); // 보수적으로 더 큰 값을 채택
	float Remaining = FMath::Max(0.f, RawDamage);

	float AbsorbedByShield = 0.f;
	if (Shield > 0.f && Remaining > 0.f)
	{
		const float Absorb = FMath::Min(Shield, Remaining);
		Shield = FMath::Max(0.f, Shield - Absorb);
		Remaining = FMath::Max(0.f, Remaining - Absorb);
		AbsorbedByShield = Absorb;

		OnRep_Shield(); // UI 갱신
		ForceNetUpdate();
	}
	
	const int32 HpDamage = FMath::Max(0, FMath::RoundToInt(Remaining));
	if (HpDamage <= 0 || IsDead())
	{
		UE_LOG(LogTemp, Verbose, TEXT("%s absorbed %.0f with Shield (Shield: %.0f/%.0f)"),
			*GetName(), AbsorbedByShield, Shield, MaxShield);
		return 0.f;
	}
	// HP 감소
	const int32 NewHP = FMath::Clamp(CurrentHP - HpDamage, 0, MaxHP);
	
	UE_LOG(LogTemp, Verbose, TEXT("%s took %d HP damage (absorbed %.0f by Shield, HP: %d -> %d, Shield: %.0f/%.0f)"),
		*GetName(), HpDamage, AbsorbedByShield, CurrentHP, NewHP, Shield, MaxShield);
	// 죽음 처리
	if (NewHP != CurrentHP)
	{
		CurrentHP = NewHP;

		OnRep_CurrentHP();

		if (IsDead())
		{
			HandleDeath(DamageCauser);
		}
		ForceNetUpdate();      // 선택: 복제 즉시 전송 유도
	}
	return HpDamage;
}

void APawnBase::SetMaxShield(float NewMax, bool bClampCurrent)
{
	if (!HasAuthority()) return;
	MaxShield = FMath::Max(0.f, NewMax);
	if (bClampCurrent)
	{
		Shield = FMath::Clamp(Shield, 0.f, MaxShield);
	}
	OnRep_Shield();
}

void APawnBase::AddShield(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f) return;
	Shield = FMath::Clamp(Shield + Amount, 0.f, MaxShield);
	OnRep_Shield();
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
	}

	UpdateHealthWidget();
}

void APawnBase::Destroyed() {
	if (Board) Board->UnRegisterPawn(this);
	Super::Destroyed();
}