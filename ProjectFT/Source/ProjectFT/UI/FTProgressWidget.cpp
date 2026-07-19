// Fill out your copyright notice in the Description page of Project Settings.

#include "FTProgressWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

#include "ProjectFT/Components/FTCaptureEscapeComponent.h"
#include "ProjectFT/Components/FTInteractionComponent.h"

namespace
{
	constexpr float ProgressPlusAnimationSeconds = 3.0f;
	constexpr float ProgressPlusRiseDistance = 42.0f;
	const FLinearColor ShelfProgressColor = FLinearColor(FColor::FromHex(TEXT("00BCFFFF")));
	const FLinearColor EscapeProgressColor = FLinearColor(FColor::FromHex(TEXT("FFA663FF")));
}

void UFTProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetRenderOpacity(0.0f);
	SetTimingWidgetsVisible(false);
	SetWidgetOptionalVisibility(TXT_StruggleHint, false);
	SetWidgetOptionalVisibility(TXT_ProgressPlusValue, false);
}

void UFTProgressWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UFTInteractionComponent* InteractionComponent = ResolveInteractionComponent();
	UFTCaptureEscapeComponent* CaptureEscapeComponent = ResolveCaptureEscapeComponent();
	const bool bIsEscaping = CaptureEscapeComponent && CaptureEscapeComponent->IsCaptured();
	const bool bIsChanneling = InteractionComponent && InteractionComponent->IsChanneling();
	SetRenderOpacity((bIsEscaping || bIsChanneling) ? 1.0f : 0.0f);

	if (bIsEscaping)
	{
		SetProgressMode(EFTProgressWidgetModeType::Escape);
		UpdateCaptureEscapeProgress(CaptureEscapeComponent);
		SetTimingWidgetsVisible(false);
		SetWidgetOptionalVisibility(TXT_StruggleHint, true);
		UpdateStruggleHint();
		UpdateProgressPlusFeedback(InDeltaTime);
		return;
	}

	if (!bIsChanneling)
	{
		SetProgressMode(EFTProgressWidgetModeType::None);
		SetTimingWidgetsVisible(false);
		SetWidgetOptionalVisibility(TXT_StruggleHint, false);
		UpdateProgressPlusFeedback(InDeltaTime);
		return;
	}

	SetProgressMode(EFTProgressWidgetModeType::ShelfSearch);
	SetWidgetOptionalVisibility(TXT_StruggleHint, false);
	UpdateChannelProgress(InteractionComponent);
	UpdateTimingWidgets(InteractionComponent);
	UpdateProgressPlusFeedback(InDeltaTime);
}

UFTInteractionComponent* UFTProgressWidget::ResolveInteractionComponent()
{
	if (CachedInteractionComponent)
	{
		return CachedInteractionComponent;
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return nullptr;
	}

	CachedInteractionComponent = OwningPawn->FindComponentByClass<UFTInteractionComponent>();
	return CachedInteractionComponent;
}

UFTCaptureEscapeComponent* UFTProgressWidget::ResolveCaptureEscapeComponent()
{
	if (CachedCaptureEscapeComponent)
	{
		return CachedCaptureEscapeComponent;
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return nullptr;
	}

	CachedCaptureEscapeComponent = OwningPawn->FindComponentByClass<UFTCaptureEscapeComponent>();
	return CachedCaptureEscapeComponent;
}

void UFTProgressWidget::UpdateChannelProgress(UFTInteractionComponent* InteractionComponent)
{
	const float Progress = InteractionComponent ? InteractionComponent->GetChannelProgress() : 0.0f;

	if (ChanneledProgressBar)
	{
		ChanneledProgressBar->SetPercent(Progress);
	}

	if (TXT_ProgressValue)
	{
		const int32 ProgressPercent = FMath::RoundToInt(Progress * 100.0f);
		TXT_ProgressValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), ProgressPercent)));
	}

	if (InteractionComponent)
	{
		const int32 RewardSerial = InteractionComponent->GetSkillCheckRewardSerial();
		if (RewardSerial != LastHandledRewardSerial)
		{
			LastHandledRewardSerial = RewardSerial;
			StartProgressPlusFeedback(InteractionComponent->GetLastSkillCheckProgressBonus());
		}
	}
}

void UFTProgressWidget::UpdateCaptureEscapeProgress(UFTCaptureEscapeComponent* CaptureEscapeComponent)
{
	const float Progress = CaptureEscapeComponent ? CaptureEscapeComponent->GetEscapeProgress() : 0.0f;

	if (ChanneledProgressBar)
	{
		ChanneledProgressBar->SetPercent(Progress);
	}

	if (TXT_ProgressValue)
	{
		const int32 ProgressPercent = FMath::RoundToInt(Progress * 100.0f);
		TXT_ProgressValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), ProgressPercent)));
	}
}

void UFTProgressWidget::UpdateTimingWidgets(UFTInteractionComponent* InteractionComponent)
{
	const bool bIsSkillCheckActive = InteractionComponent && InteractionComponent->IsSkillCheckActive();
	SetTimingWidgetsVisible(bIsSkillCheckActive);

	UProgressBar* TimingTrack = TimingBar ? TimingBar.Get() : EscapeProgressBar.Get();
	if (!bIsSkillCheckActive || !TimingTrack)
	{
		return;
	}

	TimingTrack->SetPercent(1.0f);

	UCanvasPanelSlot* TrackSlot = Cast<UCanvasPanelSlot>(TimingTrack->Slot);
	if (!TrackSlot)
	{
		return;
	}

	const FVector2D TrackPosition = TrackSlot->GetPosition();
	const FVector2D TrackSize = TrackSlot->GetSize();
	const float TrackWidth = FMath::Max(TrackSize.X, 1.0f);

	const float CursorPercent = FMath::Clamp(InteractionComponent->GetSkillCheckCursor(), 0.0f, 1.0f);
	const float TargetPercent = FMath::Clamp(InteractionComponent->GetSkillCheckTarget(), 0.0f, 1.0f);
	const float SuccessHalfWidth = FMath::Clamp(InteractionComponent->GetSkillCheckSuccessHalfWidth(), 0.0f, 0.5f);

	if (UCanvasPanelSlot* SuccessSlot = B_TimingSuccessZone ? Cast<UCanvasPanelSlot>(B_TimingSuccessZone->Slot) : nullptr)
	{
		const float SuccessWidth = FMath::Max(TrackWidth * SuccessHalfWidth * 2.0f, 4.0f);
		SuccessSlot->SetSize(FVector2D(SuccessWidth, TrackSize.Y));
		SuccessSlot->SetPosition(FVector2D(
			TrackPosition.X + TrackWidth * TargetPercent - SuccessWidth * 0.5f,
			TrackPosition.Y));
	}

	if (UCanvasPanelSlot* CursorSlot = TXT_TimingCursor ? Cast<UCanvasPanelSlot>(TXT_TimingCursor->Slot) : nullptr)
	{
		const FVector2D CursorSize = CursorSlot->GetSize();
		CursorSlot->SetPosition(FVector2D(
			TrackPosition.X + TrackWidth * CursorPercent - CursorSize.X * 0.5f,
			TrackPosition.Y - CursorSize.Y));
	}
}

void UFTProgressWidget::SetTimingWidgetsVisible(bool bVisible)
{
	SetWidgetOptionalVisibility(EscapeProgressBar, bVisible);
	SetWidgetOptionalVisibility(TimingBar, bVisible);
	SetWidgetOptionalVisibility(B_TimingSuccessZone, bVisible);
	SetWidgetOptionalVisibility(TXT_TimingCursor, bVisible);
	SetWidgetOptionalVisibility(TXT_TimingArrows, bVisible);
	SetWidgetOptionalVisibility(TXT_SpaceKey, bVisible);
}

void UFTProgressWidget::SetWidgetOptionalVisibility(UWidget* Widget, bool bVisible) const
{
	if (Widget)
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UFTProgressWidget::SetProgressMode(EFTProgressWidgetModeType NewMode)
{
	if (CurrentMode == NewMode)
	{
		return;
	}

	CurrentMode = NewMode;

	switch (CurrentMode)
	{
	case EFTProgressWidgetModeType::ShelfSearch:
		if (TXT_ProgressTitle)
		{
			TXT_ProgressTitle->SetText(FText::FromString(TEXT("선반 탐색 중")));
		}
		if (TXT_ProgressLabel)
		{
			TXT_ProgressLabel->SetText(FText::FromString(TEXT("탐색 진행도")));
		}
		if (ChanneledProgressBar)
		{
			ChanneledProgressBar->SetFillColorAndOpacity(ShelfProgressColor);
		}
		break;

	case EFTProgressWidgetModeType::Escape:
		if (TXT_ProgressTitle)
		{
			TXT_ProgressTitle->SetText(FText::FromString(TEXT("탈출 하기")));
		}
		if (TXT_ProgressLabel)
		{
			TXT_ProgressLabel->SetText(FText::FromString(TEXT("탈출 진행도")));
		}
		if (ChanneledProgressBar)
		{
			ChanneledProgressBar->SetFillColorAndOpacity(EscapeProgressColor);
		}
		break;

	default:
		break;
	}
}

void UFTProgressWidget::StartProgressPlusFeedback(float ProgressBonus)
{
	if (!TXT_ProgressPlusValue || ProgressBonus <= 0.0f)
	{
		return;
	}

	if (UCanvasPanelSlot* PlusSlot = Cast<UCanvasPanelSlot>(TXT_ProgressPlusValue->Slot))
	{
		if (!bHasPlusValueBasePosition)
		{
			PlusValueBasePosition = PlusSlot->GetPosition();
			bHasPlusValueBasePosition = true;
		}
		PlusSlot->SetPosition(PlusValueBasePosition);
	}

	const int32 BonusPercent = FMath::RoundToInt(ProgressBonus * 100.0f);
	TXT_ProgressPlusValue->SetText(FText::FromString(FString::Printf(TEXT("+%d%%"), BonusPercent)));
	TXT_ProgressPlusValue->SetRenderOpacity(1.0f);
	SetWidgetOptionalVisibility(TXT_ProgressPlusValue, true);
	PlusValueAnimationTime = ProgressPlusAnimationSeconds;
}

void UFTProgressWidget::UpdateProgressPlusFeedback(float DeltaTime)
{
	if (!TXT_ProgressPlusValue || PlusValueAnimationTime <= 0.0f)
	{
		return;
	}

	PlusValueAnimationTime = FMath::Max(0.0f, PlusValueAnimationTime - DeltaTime);
	const float Alpha = 1.0f - (PlusValueAnimationTime / ProgressPlusAnimationSeconds);

	TXT_ProgressPlusValue->SetRenderOpacity(1.0f - Alpha);
	if (UCanvasPanelSlot* PlusSlot = Cast<UCanvasPanelSlot>(TXT_ProgressPlusValue->Slot))
	{
		PlusSlot->SetPosition(PlusValueBasePosition + FVector2D(0.0f, -ProgressPlusRiseDistance * Alpha));
	}

	if (PlusValueAnimationTime <= 0.0f)
	{
		SetWidgetOptionalVisibility(TXT_ProgressPlusValue, false);
	}
}

void UFTProgressWidget::UpdateStruggleHint()
{
	if (!TXT_StruggleHint)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const float TimeSeconds = World ? World->GetTimeSeconds() : 0.0f;
	const bool bHighlightA = FMath::FloorToInt(TimeSeconds * 2.5f) % 2 == 0;
	TXT_StruggleHint->SetText(FText::FromString(bHighlightA ? TEXT("[A]    D") : TEXT("A    [D]")));

	const float Pulse = 0.75f + 0.25f * (0.5f + 0.5f * FMath::Sin(TimeSeconds * 12.0f));
	TXT_StruggleHint->SetRenderOpacity(Pulse);
}
