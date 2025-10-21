// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile/TileType.h"
#include "Board.generated.h"

class ATileActor;
class APawnBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitSelected, APawnBase*, Unit);

UCLASS()
class UTBG_API ABoard : public AActor
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugBoardLogs = true;

	ABoard();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** ���� �׸��� ��� */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void RegisterPawn(APawnBase* Pawn);
	void UnRegisterPawn(APawnBase* Pawn);
	void EnsureGridArrays();

	/** �ൿ, �ൿ ���� ��� */
	bool TryAttackUnit(APawnBase* Attacker, APawnBase* Target);
	bool TryMoveUnit(APawnBase* Unit, const FIntPoint& Target);
	void ComputeAttackables(APawnBase* Unit, TSet<FIntPoint>& Out) const;
	void ComputeMovables(APawnBase* Unit, TSet<FIntPoint>& Out);

	/** ��ǥ ��ȯ */
	UFUNCTION(BlueprintPure)
	FVector   GridToWorld(const FIntPoint& Grid) const;
	UFUNCTION(BlueprintPure)
	FIntPoint WorldToGrid(const FVector& WorldLoc) const;

	/** �̵� ��Ƽ ĳ��Ʈ */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastUnitMoved(APawnBase* Unit, FIntPoint From, FIntPoint To);

	/** �Է� ó�� */
	void HandleTileClicked(class ATileActor* Tile);

	UFUNCTION(BlueprintPure, Category = "Turn|Debug")
	int32 GetRemainingAP() const;


	// ���� �غ� ����(�׸���/Ÿ�� �¾� �Ϸ� Ȯ��) ���� ��� �Ǵܿ�
	UFUNCTION(BlueprintPure, Category = "Board|Grid")
	bool IsGridReady() const;

	// ���� �� ���� ��ƿ(Prev ���� New�� ��ġ)  ����/Ŭ�� ����
	void UpdatePawnCell(APawnBase* Pawn, const FIntPoint& Prev, const FIntPoint& New);

	// ���� ���� �̺�Ʈ(����  HUD)
	UPROPERTY(BlueprintAssignable, Category = "Board|Events")
	FOnUnitSelected OnUnitSelected;

protected:
	virtual void BeginPlay() override;

	// �׸��� ũ��, Ÿ�� ũ��, ����, Ÿ�� Ŭ����
	UPROPERTY(EditAnywhere, Category = "Config")
	int32 Rows = 8;
	UPROPERTY(EditAnywhere, Category = "Config")
	int32 Cols = 8;
	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = 1.0))
	float TileSize = 100.f;
	UPROPERTY(EditAnywhere, Category = "Config")
	FVector BoardOrigin = FVector::ZeroVector;
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<ATileActor> TileClass;

	// BP���� ���� �������� Ÿ�� �迭 (���� �ν��Ͻ����� ä��)
	UPROPERTY(EditInstanceOnly, Category = "Setup")
	TArray<TObjectPtr<ATileActor>> PlacedTiles;

	// BP���� ���� �������� �� �迭 (���� �ν��Ͻ����� ä��)
	UPROPERTY(EditInstanceOnly, Category = "Setup")
	TArray<TObjectPtr<APawnBase>> PlacedPawns;

	UFUNCTION(BlueprintCallable, Category = "Setup")
	void RebuildGridFromManualTiles();

private:
	// ���� �ӽ� ����
	EBoardState BoardState = EBoardState::EBS_Idle;
	void EnterIdle();
	void EnterUnitSelected(APawnBase* Unit);

	// ���̶���Ʈ ����
	void HighlightTile(const FIntPoint& Tile, EHighlightType Type);
	void HighlightTiles(const TArray<FIntPoint>& Tiles, EHighlightType Type);
	void HighlightTiles(const TSet<FIntPoint>& Tiles, EHighlightType Type);
	void ClearAllHighlights();
	void UpdateHighLights();

	/** �� ����(�������� GameState�� ����) */
	void EndTurn();

	/** ���� �׸��� ����� */
	UPROPERTY()
	TArray<TObjectPtr<ATileActor>> TileGrid;
	UPROPERTY()
	TArray<TObjectPtr<APawnBase>> PawnGrid;
	
	// ���� ���õ� Ÿ��, ����
	UPROPERTY()
	TObjectPtr<ATileActor> SelectedTile = nullptr;
	UPROPERTY() 
	APawnBase* SelectedUnit = nullptr;

	// ������ �� �ִ� ������ �� �ִ� Ÿ�� �ӽ� ��Ʈ
	UPROPERTY()
	TSet<FIntPoint> MovableTileSet;
	UPROPERTY()
	TSet<FIntPoint> AttackableTileSet;

	// ���� �غ� �� Pawn ��� ��û ����(���� ��Ͽ�)
	UPROPERTY()
	TArray<TWeakObjectPtr<APawnBase>> PendingRegister;

	// ����
	static int32 Manhattan(const FIntPoint& A, const FIntPoint& B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}

public:
	UFUNCTION(BlueprintPure, Category = "Board|Grid")
	FORCEINLINE bool IsValidCoord(const FIntPoint& Grid) const { return Grid.X >= 0 && Grid.X < Cols && Grid.Y >= 0 && Grid.Y < Rows; }
	UFUNCTION(BlueprintPure, Category = "Board|Grid")
	FORCEINLINE int32 ToIndex(const FIntPoint& Grid) const { return Grid.Y * Cols + Grid.X; }
	UFUNCTION(BlueprintPure, Category = "Board|Grid")
	FORCEINLINE ATileActor* GetTile(const FIntPoint& Grid) const { return IsValidCoord(Grid) ? TileGrid[ToIndex(Grid)] : nullptr; }
};
