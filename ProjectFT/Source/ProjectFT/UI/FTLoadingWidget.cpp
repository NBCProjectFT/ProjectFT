// Fill out your copyright notice in the Description page of Project Settings.


#include "FTLoadingWidget.h"


#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UFTLoadingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UFTLoadingWidget::SetPercent(float Percent) const
{
	if(PB_LoadingBar != nullptr)
	{
		PB_LoadingBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UFTLoadingWidget::ReadyToStart() const
{
	if(PB_LoadingBar)
		PB_LoadingBar->SetPercent(1.f);
	if(TB_Text)
		TB_Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	if(BTN_Click)
		BTN_Click->SetVisibility(ESlateVisibility::Visible);
	if(TB_ObjectName)
		TB_ObjectName->SetVisibility(ESlateVisibility::Collapsed);
}

bool UFTLoadingWidget::BindOnButtonClicked(TFunction<void()> InCallback)
{
	if (!BTN_Click) return false;

	StoredCallback = MoveTemp(InCallback);

	BTN_Click->OnClicked.RemoveAll(this);// 중복 방지
	BTN_Click->OnClicked.AddDynamic(this, &UFTLoadingWidget::HandleButtonClicked);
	return true;
}

void UFTLoadingWidget::SetObjectName(const FString& ObjectName, const int32& CompleteNum, const int32& TotalNum) const
{
	if (TB_ObjectCounts)
	{
		const FText Template = FText::FromString(TEXT("{0} / {1}"));
		const FText Display = FText::Format(
			Template,
			FText::AsNumber(CompleteNum),
			FText::AsNumber(TotalNum));

		TB_ObjectCounts->SetVisibility(ESlateVisibility::Visible);
		TB_ObjectCounts->SetText(Display);
	}
	if(TB_ObjectName)
	{
		TB_ObjectName->SetVisibility(ESlateVisibility::Visible);
		TB_ObjectName->SetText(FText::FromString(ObjectName));
	}
}

void UFTLoadingWidget::HandleButtonClicked()
{
	if (StoredCallback.IsSet())
	{
		StoredCallback.GetValue()();
	}
}
