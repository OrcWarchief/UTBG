// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/PipWidget.h"
#include "Components/Image.h"

void UPipWidget::SetFilled(bool bFilled)
{
    if (Image_Dot)
    {
        Image_Dot->SetColorAndOpacity(bFilled ? FilledColor : EmptyColor);
    }
}