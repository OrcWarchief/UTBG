// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PipBoxWidget.generated.h"

class UHorizontalBox;
class UPipWidget;
/**
 * 
 */
UCLASS()
class UTBG_API UPipBoxWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "Pips")
    void SetCounts(int32 Current, int32 Max);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pips")
    TSubclassOf<UPipWidget> PipClass;

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHorizontalBox> HBox_Pips;

private:
    void EnsureChildren(int32 Max);
};
