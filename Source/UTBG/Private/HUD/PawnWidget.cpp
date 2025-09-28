// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/PawnWidget.h"
#include "Components/TextBlock.h"

void UPawnWidget::SetHealth(int32 Current, int32 Max)
{
    if (Health)
        Health->SetText(FText::AsNumber(Current));

    if (MaxHealth)
        MaxHealth->SetText(FText::AsNumber(Max));
}