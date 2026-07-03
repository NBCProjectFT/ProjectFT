// Fill out your copyright notice in the Description page of Project Settings.


#include "FTCountdownEscapeWidget.h"

#include "Components/TextBlock.h"

void UFTCountdownEscapeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResetCountdown();
}

void UFTCountdownEscapeWidget::SetRemainingTime(float NewRemainingTime)
{
	RemainingTime = FMath::Max(0.0f, NewRemainingTime);

	// 2.1 seconds is displayed as 3, while 1.0 is displayed as 1.
	// This keeps the countdown readable and gives WBP a clean moment to play its number-change animation.
	const int32 DisplaySecond = FMath::CeilToInt(RemainingTime);
	UpdateCountdownText(DisplaySecond);

	if (DisplaySecond != LastDisplayedSecond)
	{
		LastDisplayedSecond = DisplaySecond;
		OnCountdownSecondChanged(DisplaySecond);
	}
}

void UFTCountdownEscapeWidget::ResetCountdown()
{
	RemainingTime = 0.0f;
	LastDisplayedSecond = INDEX_NONE;
	UpdateCountdownText(0);
}

void UFTCountdownEscapeWidget::RequestCountdownMessageBroadcast(float NewRemainingTime)
{
	// Future connection point:
	// - Broadcast an escape countdown/progress gameplay message.
	// - Put the remaining time or progress percent in the payload.
	OnCountdownMessageRequested(NewRemainingTime);
}

void UFTCountdownEscapeWidget::UpdateCountdownText(int32 DisplaySecond)
{
	if (!CountDownText)
	{
		return;
	}

	CountDownText->SetText(FText::AsNumber(DisplaySecond));
}
