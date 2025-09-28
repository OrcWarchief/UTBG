// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PawnWidget.generated.h"

class UTextBlock;
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

	UPROPERTY(meta = (BindWidget)) 
	UTextBlock* Health;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MaxHealth;
	
};
