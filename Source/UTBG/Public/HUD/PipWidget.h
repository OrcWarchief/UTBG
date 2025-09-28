// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PipWidget.generated.h"

class UImage;

/**
 * 
 */
UCLASS()
class UTBG_API UPipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Pip")
    void SetFilled(bool bFilled);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Image_Dot;

    UPROPERTY(EditAnywhere, Category = "Pip")
    FLinearColor FilledColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

    UPROPERTY(EditAnywhere, Category = "Pip")
    FLinearColor EmptyColor = FLinearColor(1.f, 1.f, 1.f, 0.25f);
};
