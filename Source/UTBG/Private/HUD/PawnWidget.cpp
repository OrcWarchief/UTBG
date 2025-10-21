// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/PawnWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Overlay.h"

void UPawnWidget::SetHealth(int32 Current, int32 Max)
{
    CachedHP = Current;
    CachedHPMax = Max;

    if (!PB_HP || !TB_HP) return;

    const float Pct = (Max > 0) ? (float)Current / (float)Max : 0.f;

    PB_HP->SetPercent(FMath::Clamp(Pct, 0.f, 1.f));
    TB_HP->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Current, Max)));
}

void UPawnWidget::SetShield(float Current, float Max)
{
    CachedShield = Current;
    CachedShieldMax = Max;

    if (!PB_Shield || !TB_Shield || !O_ShieldRow) return;

    if (CurrentLOD < 2)
    {
        O_ShieldRow->SetVisibility(ESlateVisibility::Collapsed);
        PrevShield = -1.f;
        return;
    }

    if (Max <= 0.f || Current <= 0.f)
    {
        O_ShieldRow->SetVisibility(ESlateVisibility::Collapsed);
        PB_Shield->SetPercent(0.f);
        TB_Shield->SetText(FText::GetEmpty());
        PrevShield = -1.f;
        return;
    }

    O_ShieldRow->SetVisibility(ESlateVisibility::Visible);
    const float Pct = FMath::Clamp(Current / Max, 0.f, 1.f);
    PB_Shield->SetPercent(Pct);
    TB_Shield->SetText(FText::AsNumber(FMath::RoundToInt(Current)));

    if (PrevShield >= 0.f)
    {
        if (Current <= 0.f && PrevShield > 0.f)
        {
            PlayShieldBreak();
        }
        else if (Current < PrevShield)
        {
            PlayShieldHit();
        }
    }
    PrevShield = Current;
}

void UPawnWidget::PlayShieldHit()
{
    if (Anim_ShieldHit) PlayAnimation(Anim_ShieldHit, 0.f, 1);
}

void UPawnWidget::PlayShieldBreak()
{
    if (Anim_ShieldBreak) PlayAnimation(Anim_ShieldBreak, 0.f, 1);
}

void UPawnWidget::SetCompactModeByLOD(int32 InLOD)
{
    CurrentLOD = FMath::Clamp(InLOD, 0, 2);
    ApplyLODVisibility();

    // Re-apply cached values so the widget paints the right state under new LOD
    if (CachedHPMax >= 0 && CachedHP >= 0)
    {
        // Re-feed text for TB visibility change
        SetHealth(CachedHP, CachedHPMax);
    }
    if (CachedShieldMax >= 0.f && CachedShield >= 0.f)
    {
        SetShield(CachedShield, CachedShieldMax);
    }
}

void UPawnWidget::ApplyLODVisibility()
{
    // HP number: hidden only at LOD 0
    if (TB_HP)
    {
        TB_HP->SetVisibility(CurrentLOD >= 1
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed);
    }

    // Shield number text is only relevant at LOD 2
    if (TB_Shield)
    {
        TB_Shield->SetVisibility(CurrentLOD >= 2
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed);
    }
}