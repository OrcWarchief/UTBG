// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile/TileType.h"
#include "TileActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class ABoard;       // ��ǥ����� ��ȯ�� ����
class ATileActor;  // ��������Ʈ �Ķ���Ϳ� ���� ����
class APawnBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTileClicked, ATileActor*, Tile);

UCLASS()
class UTBG_API ATileActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATileActor();

	// �����Ϳ��� �ν��Ͻ����� ���� X,Y�� ���� -1�� �� ���� ��
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "-1"))
	FIntPoint GridCoord = FIntPoint(-1, -1);

	// ���̶���Ʈ
	UFUNCTION(BlueprintCallable, Category = "Highlight")
	void Highlight(EHighlightType Type);

	UFUNCTION(BlueprintCallable, Category = "Highlight")
	void ClearHighlight();

	UPROPERTY(BlueprintAssignable)
	FTileClicked OnTileClicked;

protected:
	virtual void BeginPlay() override;
	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

private:

	// �޽� ������Ʈ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TileMesh;
	// ���̶���Ʈ �� ���� ���� �ٸ��� Ÿ���� ���̶���Ʈ ��
	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	UMaterialInterface* BaseMaterial;      // �⺻

	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	UMaterialInterface* MoveMaterial;           // �̵� ������

	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	UMaterialInterface* AttackMaterial;         // ���� ������

	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	UMaterialInterface* SelectedMaterial;       // ���� ��

	UPROPERTY()
	EHighlightType CurrentHighlight = EHighlightType::EHT_None;

public:	
	// --- ������/������: �ִ��� �ζ��� ---
	UFUNCTION(BlueprintPure, Category = "Grid")
	FORCEINLINE FIntPoint GetGridCoord() const { return GridCoord; }

	// ��ǥ �ٲٸ� ��� ���� ��ġ�� ����.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FORCEINLINE void SetGridCoord(FIntPoint In) { GridCoord = In; }

	UFUNCTION(BlueprintPure, Category = "Tile")
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return TileMesh; }
};
