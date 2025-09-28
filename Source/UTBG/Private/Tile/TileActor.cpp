// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile/TileActor.h"
#include "Board/Board.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Tile/TileType.h"

ATileActor::ATileActor()
{
	PrimaryActorTick.bCanEverTick = false;   // Ÿ���� �Ϲ������� ƽ ���ʿ�

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	SetRootComponent(TileMesh);

	if (TileMesh)
	{
		TileMesh->SetGenerateOverlapEvents(false); // ���� Ʈ���̽��� ������ �� �� 
		TileMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TileMesh->SetCollisionObjectType(ECC_WorldStatic);
		TileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		TileMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		TileMesh->bReceivesDecals = true; // ź��, ����Ʈ, ���̶���Ʈ�� ��Į�� �Ѹ� ��
	}
}

void ATileActor::Highlight(EHighlightType Type)
{
	if (!TileMesh) return;
	UE_LOG(LogTemp, Warning, TEXT("HighLight"));
	if (CurrentHighlight == Type) return;

	CurrentHighlight = Type;
	UMaterialInterface* TargetMaterial = BaseMaterial;

	switch (Type)
	{
	case EHighlightType::EHT_Movable:		TargetMaterial = MoveMaterial;		break;
	case EHighlightType::EHT_Attackable:	TargetMaterial = AttackMaterial;	break;
	case EHighlightType::EHT_Selected:		TargetMaterial = SelectedMaterial;	break;
	case EHighlightType::EHT_None:
	default: /* None */						TargetMaterial = BaseMaterial;		break;
	}

	if (TargetMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("HighLight TargetMaterial"));
		TileMesh->SetMaterial(0, TargetMaterial);
	}
}

void ATileActor::ClearHighlight()
{
	CurrentHighlight = EHighlightType::EHT_None;
	if (TileMesh && BaseMaterial)
	{
		TileMesh->SetMaterial(0, BaseMaterial);
	}
}

void ATileActor::BeginPlay()
{
	Super::BeginPlay();

	if (TileMesh && BaseMaterial && CurrentHighlight == EHighlightType::EHT_None)
	{
		TileMesh->SetMaterial(0, BaseMaterial);
	}
}

void ATileActor::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);
	OnTileClicked.Broadcast(this); // �ڱ� �ڽ� ������ ����
}
