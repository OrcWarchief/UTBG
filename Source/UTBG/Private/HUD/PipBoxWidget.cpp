// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/PipBoxWidget.h"
#include "HUD/PipWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UPipBoxWidget::SetCounts(int32 Current, int32 Max)
{
    if (!HBox_Pips) return;
    EnsureChildren(Max);

    const int32 Count = HBox_Pips->GetChildrenCount();
    for (int32 i = 0; i < Count; ++i)
    {
        if (UPipWidget* Pip = Cast<UPipWidget>(HBox_Pips->GetChildAt(i)))
        {
            Pip->SetFilled(i < Current);
        }
    }
}

void UPipBoxWidget::EnsureChildren(int32 Max)
{
    if (!HBox_Pips) return;
    if (!PipClass) return;

    const int32 Cur = HBox_Pips->GetChildrenCount();

    // 부족하면 생성
    for (int32 i = Cur; i < Max; ++i)
    {
        UPipWidget* Pip = CreateWidget<UPipWidget>(this, PipClass);
        if (Pip)
        {
            UHorizontalBoxSlot* HorizontalBoxSlot = HBox_Pips->AddChildToHorizontalBox(Pip);
            if (HorizontalBoxSlot)
            {
                HorizontalBoxSlot->SetPadding(FMargin(2.f, 0.f));
                HorizontalBoxSlot->SetHorizontalAlignment(HAlign_Fill);
                HorizontalBoxSlot->SetVerticalAlignment(VAlign_Fill);
            }
        }
    }

    // 너무 많으면 제거
    for (int32 i = HBox_Pips->GetChildrenCount() - 1; i >= Max; --i)
    {
        HBox_Pips->RemoveChildAt(i);
    }
}
