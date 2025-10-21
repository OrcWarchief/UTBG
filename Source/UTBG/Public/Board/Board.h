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

	/** 유닛 그리드 등록 */
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void RegisterPawn(APawnBase* Pawn);
	void UnRegisterPawn(APawnBase* Pawn);
	void EnsureGridArrays();

	/** 행동, 행동 범위 계산 */
	bool TryAttackUnit(APawnBase* Attacker, APawnBase* Target);
	bool TryMoveUnit(APawnBase* Unit, const FIntPoint& Target);
	void ComputeAttackables(APawnBase* Unit, TSet<FIntPoint>& Out) const;
	void ComputeMovables(APawnBase* Unit, TSet<FIntPoint>& Out);

	/** 좌표 변환 */
	UFUNCTION(BlueprintPure)
	FVector   GridToWorld(const FIntPoint& Grid) const;
	UFUNCTION(BlueprintPure)
	FIntPoint WorldToGrid(const FVector& WorldLoc) const;

	/** 이동 멀티 캐스트 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastUnitMoved(APawnBase* Unit, FIntPoint From, FIntPoint To);

	/** 입력 처리 */
	void HandleTileClicked(class ATileActor* Tile);

	UFUNCTION(BlueprintPure, Category = "Turn|Debug")
	int32 GetRemainingAP() const;


	// 보드 준비 여부(그리드/타일 셋업 완료 확인) 지연 등록 판단용
	UFUNCTION(BlueprintPure, Category = "Board|Grid")
	bool IsGridReady() const;

	// 점유 맵 갱신 유틸(Prev 비우고 New에 배치)  서버/클라 공용
	void UpdatePawnCell(APawnBase* Pawn, const FIntPoint& Prev, const FIntPoint& New);

	// 유닛 선택 이벤트(보드  HUD)
	UPROPERTY(BlueprintAssignable, Category = "Board|Events")
	FOnUnitSelected OnUnitSelected;

protected:
	virtual void BeginPlay() override;

	// 그리드 크기, 타일 크기, 원점, 타일 클래스
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

	// BP에서 직접 끼워넣을 타일 배열 (레벨 인스턴스에서 채움)
	UPROPERTY(EditInstanceOnly, Category = "Setup")
	TArray<TObjectPtr<ATileActor>> PlacedTiles;

	// BP에서 직접 끼워넣을 폰 배열 (레벨 인스턴스에서 채움)
	UPROPERTY(EditInstanceOnly, Category = "Setup")
	TArray<TObjectPtr<APawnBase>> PlacedPawns;

	UFUNCTION(BlueprintCallable, Category = "Setup")
	void RebuildGridFromManualTiles();

private:
	// 상태 머신 관련
	EBoardState BoardState = EBoardState::EBS_Idle;
	void EnterIdle();
	void EnterUnitSelected(APawnBase* Unit);

	// 하이라이트 관련
	void HighlightTile(const FIntPoint& Tile, EHighlightType Type);
	void HighlightTiles(const TArray<FIntPoint>& Tiles, EHighlightType Type);
	void HighlightTiles(const TSet<FIntPoint>& Tiles, EHighlightType Type);
	void ClearAllHighlights();
	void UpdateHighLights();

	/** 턴 종료(서버에서 GameState에 위임) */
	void EndTurn();

	/** 내부 그리드 저장소 */
	UPROPERTY()
	TArray<TObjectPtr<ATileActor>> TileGrid;
	UPROPERTY()
	TArray<TObjectPtr<APawnBase>> PawnGrid;
	
	// 현재 선택된 타일, 유닛
	UPROPERTY()
	TObjectPtr<ATileActor> SelectedTile = nullptr;
	UPROPERTY() 
	APawnBase* SelectedUnit = nullptr;

	// 움직일 수 있는 공격할 수 있는 타일 임시 세트
	UPROPERTY()
	TSet<FIntPoint> MovableTileSet;
	UPROPERTY()
	TSet<FIntPoint> AttackableTileSet;

	// 보드 준비 전 Pawn 등록 요청 보관(지연 등록용)
	UPROPERTY()
	TArray<TWeakObjectPtr<APawnBase>> PendingRegister;

	// 헬퍼
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
