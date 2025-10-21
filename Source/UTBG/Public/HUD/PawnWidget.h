// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PawnWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UOverlay;
/**
 * 
 */
UCLASS()
class UTBG_API UPawnWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void SetHealth(int32 Current, int32 Max);

	UFUNCTION(BlueprintCallable)
	void SetShield(float Current, float Max);

	UFUNCTION(BlueprintCallable)
	void PlayShieldHit();       // ��� ���� ȿ��
	UFUNCTION(BlueprintCallable)
	void PlayShieldBreak();     // 0�� �Ǿ��� ��

	// ī�޶��� ������ LOD �ܰ踦 ����
	UFUNCTION(BlueprintCallable, Category = "LOD")
	void SetCompactModeByLOD(int32 InLOD);

	// ������
	UFUNCTION(BlueprintPure, Category = "LOD")
	int32 GetCurrentLOD() const { return CurrentLOD; }


protected:
	// BindWidget
	UPROPERTY(meta = (BindWidget)) UProgressBar* PB_HP;
	UPROPERTY(meta = (BindWidget)) UProgressBar* PB_Shield;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TB_HP;
	UPROPERTY(meta = (BindWidget)) UTextBlock* TB_Shield;
	UPROPERTY(meta = (BindWidget)) UOverlay* O_ShieldRow;

	// Anim
	UPROPERTY(Transient, meta = (BindWidgetAnim)) UWidgetAnimation* Anim_ShieldHit;
	UPROPERTY(Transient, meta = (BindWidgetAnim)) UWidgetAnimation* Anim_ShieldBreak;

private:
	int32  CachedHP = -1;
	int32  CachedHPMax = -1;
	float  CachedShield = -1.f;
	float  CachedShieldMax = -1.f;

	float PrevShield = -1.f;

	int32 CurrentLOD = 2;

	void ApplyLODVisibility();
};
