// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile/TileType.h"
#include "TileActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class ABoard;       // 좌표→월드 변환을 위해
class ATileActor;  // 델리게이트 파라미터용 전방 선언
class APawnBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTileClicked, ATileActor*, Tile);

UCLASS()
class UTBG_API ATileActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATileActor();

	// 에디터에서 인스턴스별로 직접 X,Y를 설정 -1은 미 지정 값
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "-1"))
	FIntPoint GridCoord = FIntPoint(-1, -1);

	// 하이라이트
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

	// 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* TileMesh;
	// 하이라이트 각 상태 별로 다르게 타일을 하이라이트 함
	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	UMaterialInterface* BaseMaterial;      // 기본

	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	UMaterialInterface* MoveMaterial;           // 이동 범위용

	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	UMaterialInterface* AttackMaterial;         // 공격 범위용

	UPROPERTY(EditDefaultsOnly, Category = "Highlight")
	UMaterialInterface* SelectedMaterial;       // 선택 중

	UPROPERTY()
	EHighlightType CurrentHighlight = EHighlightType::EHT_None;

public:	
	// --- 접근자/설정자: 최대한 인라인 ---
	UFUNCTION(BlueprintPure, Category = "Grid")
	FORCEINLINE FIntPoint GetGridCoord() const { return GridCoord; }

	// 좌표 바꾸면 즉시 월드 위치로 스냅.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FORCEINLINE void SetGridCoord(FIntPoint In) { GridCoord = In; }

	UFUNCTION(BlueprintPure, Category = "Tile")
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return TileMesh; }
};
