// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SkillBoxWidget.h"
#include "Components/Button.h"
#include "PlayerController/UTBGPlayerController.h"
#include "Data/SkillData.h"
#include "Pawn/PawnBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

DEFINE_LOG_CATEGORY(LogSkillBox);

void USkillBoxWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 디자인 모드에서만 가볍게 시각 요소 갱신 (상태 비활성화 금지)
    if (IsDesignTime())
    {
        if (TextSkill)   TextSkill->SetText(SkillData ? SkillData->DisplayName : FText::FromString(TEXT("Skill")));
        if (ImageSkill)  ImageSkill->SetBrushFromTexture(SkillData ? SkillData->Icon : nullptr);
        if (TextCooldown) TextCooldown->SetVisibility(ESlateVisibility::Collapsed);
        SetRenderOpacity(1.f);
    }
}

void USkillBoxWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // BindWidget 성공 여부를 반드시 점검
    if (!ensureMsgf(ButtonSkill, TEXT("[SkillBox] BindWidget failed: ButtonSkill is null (check UMG name).")))
    {
        return;
    }

    // 중복 방지 후 바인딩
    ButtonSkill->OnClicked.RemoveAll(this);
    ButtonSkill->OnClicked.AddUniqueDynamic(this, &USkillBoxWidget::OnButtonClicked);

    ButtonSkill->SetClickMethod(EButtonClickMethod::MouseDown);

    // (진단) 눌림/떼짐 로그
    ButtonSkill->OnPressed.AddUniqueDynamic(this, &USkillBoxWidget::OnPressed_Debug);
    ButtonSkill->OnReleased.AddUniqueDynamic(this, &USkillBoxWidget::OnReleased_Debug);

    UE_LOG(LogSkillBox, Warning, TEXT("[SkillBox] Construct: Bound OnClicked. User=%s Skill=%s"),
        *GetNameSafe(UserUnit), *GetNameSafe(SkillData));
}

void USkillBoxWidget::NativeDestruct()
{
    if (ButtonSkill)
    {
        ButtonSkill->OnClicked.RemoveAll(this);
        ButtonSkill->OnPressed.RemoveAll(this);
        ButtonSkill->OnReleased.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void USkillBoxWidget::Setup(APawnBase* InUser, USkillData* InData)
{
    UserUnit = InUser;
    SkillData = InData;

    bUsable = true;
    CachedCooldownTurns = 0;

    UE_LOG(LogSkillBox, Warning, TEXT("[SkillBox] Setup: User=%s Skill=%s"),
        *GetNameSafe(UserUnit), *GetNameSafe(SkillData));

    Refresh();
}

void USkillBoxWidget::Refresh()
{
    // 이름/아이콘
    if (TextSkill)
    {
        TextSkill->SetText(SkillData ? SkillData->DisplayName : FText::GetEmpty());
    }
    if (ImageSkill)
    {
        if (SkillData && SkillData->Icon) { ImageSkill->SetBrushFromTexture(SkillData->Icon); }
        else { ImageSkill->SetBrushFromTexture(nullptr); }
    }

    // 표기 갱신
    SetCooldownTurns(CachedCooldownTurns);
    SetUsable(bUsable);
}

void USkillBoxWidget::SetUsable(bool bInUsable)
{
    bUsable = bInUsable;
    const bool bEnable = bUsable && (CachedCooldownTurns <= 0);
    if (ButtonSkill) ButtonSkill->SetIsEnabled(bEnable);

    SetRenderOpacity(bEnable ? 1.f : 0.5f);

    UE_LOG(LogSkillBox, Verbose, TEXT("[SkillBox] SetUsable=%d (Skill=%s, CD=%d)"),
        bUsable ? 1 : 0,
        SkillData ? *SkillData->SkillId.ToString() : TEXT("None"),
        CachedCooldownTurns);
}

void USkillBoxWidget::SetCooldownTurns(int32 TurnsLeft)
{
    CachedCooldownTurns = TurnsLeft;

    if (TextCooldown)
    {
        if (TurnsLeft > 0)
        {
            TextCooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
            TextCooldown->SetText(FText::Format(FText::FromString(TEXT("CD: {0}")), FText::AsNumber(TurnsLeft)));
        }
        else
        {
            TextCooldown->SetVisibility(ESlateVisibility::Collapsed);
            TextCooldown->SetText(FText::GetEmpty());
        }
    }

    const bool bEnable = bUsable && (CachedCooldownTurns <= 0);
    if (ButtonSkill) ButtonSkill->SetIsEnabled(bEnable);
    SetRenderOpacity(bEnable ? 1.f : 0.5f);

    UE_LOG(LogSkillBox, Verbose, TEXT("[SkillBox] SetCooldownTurns=%d (Skill=%s, Usable=%d)"),
        TurnsLeft,
        SkillData ? *SkillData->SkillId.ToString() : TEXT("None"),
        bUsable ? 1 : 0);
}

void USkillBoxWidget::OnPressed_Debug()
{
    UE_LOG(LogSkillBox, Warning, TEXT("[SkillBox] Pressed (Enabled=%d)"),
        ButtonSkill ? ButtonSkill->GetIsEnabled() : -1);
    bFiredThisPress = false; // 새 클릭 사이클 시작
}

void USkillBoxWidget::OnReleased_Debug()
{
    UE_LOG(LogSkillBox, Warning, TEXT("[SkillBox] Released"));
    bFiredThisPress = false; // 다음 클릭 대비
}

void USkillBoxWidget::OnButtonClicked()
{
    UE_LOG(LogSkillBox, Warning, TEXT("[SkillBox] Clicked: User=%s SkillId=%s Usable=%d CD=%d"),
        *GetNameSafe(UserUnit),
        SkillData ? *SkillData->SkillId.ToString() : TEXT("None"),
        bUsable ? 1 : 0,
        CachedCooldownTurns);

    if (!UserUnit || !SkillData) return;
    if (!bUsable || CachedCooldownTurns > 0) return;

    if (!bFiredThisPress)
    {
        bFiredThisPress = true;
        OnSkillClicked.Broadcast(UserUnit, SkillData->SkillId);
    }
}