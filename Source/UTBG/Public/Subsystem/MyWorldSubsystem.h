// Fill out your copyright notice in the Description page of Project Settings.

//#pragma once
//
//#include "CoreMinimal.h"
//#include "Subsystems/WorldSubsystem.h"
//#include "Pawn/PawnBase.h"
//#include "Tile/TileActor.h"
//#include "Tile/TileType.h"
//#include "MyWorldSubsystem.generated.h"
//
///**
// * 
// */
//
//
//UCLASS(Blueprintable)
//class UTBG_API UMyWorldSubsystem : public UWorldSubsystem
//{
//	GENERATED_BODY()
//	
//public:
//	///* ~~ Lifecycle overrides ~~ */
//	//virtual void Initialize(FSubsystemCollectionBase& Collection) override;
//	//virtual void Deinitialize() override;
//
//	/* ~~ Board creation / teardown ~~ */
//	UFUNCTION(BlueprintCallable, Category = "Board")
//	void GenerateBoard(int32 InRows, int32 InCols);
//
//	UFUNCTION(BlueprintCallable, Category = "Board")
//	void ClearBoard();
//
//	/* ~~ Coordinate helpers ~~ */
//	UFUNCTION(BlueprintPure, Category = "Board|Coords")
//	FVector   GridToWorld(const FIntPoint& Grid) const;
//
//	UFUNCTION(BlueprintPure, Category = "Board|Coords")
//	FIntPoint WorldToGrid(const FVector& WorldLoc) const;
//
//	///* ~~ Tile queries & state ~~ */
//	//UFUNCTION(BlueprintPure, Category = "Board|Tiles")
//	//ATileActor* GetTile(const FIntPoint& Grid) const;        // nullptr if OOB or using struct mode
//
//	//UFUNCTION(BlueprintPure, Category = "Board|Tiles")
//	//bool IsWalkable(const FIntPoint& Grid) const;
//
//	//UFUNCTION(BlueprintCallable, Category = "Board|Tiles")
//	//void SetOccupyingPiece(const FIntPoint& Grid, APawnBase* Piece);
//
//	//UFUNCTION(BlueprintPure, Category = "Board|Tiles")
//	//APawnBase* GetOccupyingPiece(const FIntPoint& Grid) const;
//
//	///* ~~ Range & path helpers ~~ */
//	//UFUNCTION(BlueprintPure, Category = "Board|Tiles")
//	//TArray<ATileActor*> GetNeighborTiles(const FIntPoint& Grid, int32 Range = 1, bool bIncludeDiagonals = true) const;
//
//	//UFUNCTION(BlueprintPure, Category = "Board|Path")
//	//bool FindPath(const FIntPoint& Start, const FIntPoint& Goal, TArray<FIntPoint>& OutPath) const;
//
//	/* ~~ Visual helpers (client‑only) ~~ */
//	UFUNCTION(BlueprintCallable, Category = "Board|Visual")
//	void HighlightTiles(const TArray<FIntPoint>& Tiles, EHighlightType Type);
//
//	UFUNCTION(BlueprintCallable, Category = "Board|Visual")
//	void ResetAllHighlights(const TArray<FIntPoint>& Tiles);
//
//protected:
//
//
//private:
//	/* ------------ Editable properties (designer‑friendly) ---------------- */
//	UPROPERTY(EditAnywhere, Category = "Config")
//	int32 Rows = 8;
//
//	UPROPERTY(EditAnywhere, Category = "Config")
//	int32 Cols = 8;
//
//	/** World‑space size of one square; used by Grid↔World conversion. */
//	UPROPERTY(EditAnywhere, Category = "Config", meta = (ClampMin = 1.0))
//	float TileSize = 100.f;
//
//	// 좌측-아래(0,0)에 첫 번째 칸이 오도록 단순화
//	UPROPERTY(EditAnywhere, Category = "Config")
//	FVector BoardOrigin = FVector::ZeroVector;
//
//	/** If set, each tile will be spawned as this Actor class; otherwise FTileData mode is used. */
//	UPROPERTY(EditDefaultsOnly, Category = "Config")
//	TSubclassOf<ATileActor> TileClass;
//
//	/* ------------ Runtime storage --------------------------------------- */
//	// Flat array: Y * Cols + X (either tile actors or pure data).
//	// 각 타일에 대한 포인터 모음 배열
//	UPROPERTY(Transient)
//	TArray<TObjectPtr<ATileActor>> TileGrid;
//
//	UPROPERTY(Transient)
//	bool bGenerated = false;
//
//public:
//
//	/* Index = Y * Cols + X */
//	FORCEINLINE int32 ToIndex(const FIntPoint& Grid) const { return Grid.Y * Cols + Grid.X; }
//	/* ~~ Inline getters ~~ */
//	FORCEINLINE int32 GetRows() const { return Rows; }
//	FORCEINLINE int32 GetCols() const { return Cols; }
//	// 칸 유효한지 체크
//	FORCEINLINE bool IsValidCoord(const FIntPoint& Grid) const { return Grid.X >= 0 && Grid.X < Cols && Grid.Y >= 0 && Grid.Y < Rows; }
//};
