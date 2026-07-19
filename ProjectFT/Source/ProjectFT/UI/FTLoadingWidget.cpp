// Fill out your copyright notice in the Description page of Project Settings.


#include "FTLoadingWidget.h"


#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UFTLoadingWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	bReadyToStart = false;

	if (IMG_ReadyBlackBackground)
	{
		IMG_ReadyBlackBackground->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (BTN_Click)
	{
		BTN_Click->SetVisibility(ESlateVisibility::Collapsed);
	}
	// if (TB_Text)
	// {
	// 	TB_Text->SetVisibility(ESlateVisibility::Collapsed);
	// }
	
	// TEST CODE 로딩되는 Object 표시
	if(TB_ObjectName)
		TB_ObjectName->SetVisibility(ESlateVisibility::Hidden);
	if(TB_ObjectCounts)
		TB_ObjectCounts->SetVisibility(ESlateVisibility::Hidden);
	if(SizeBox_LoadingText)
		SizeBox_LoadingText->SetVisibility(ESlateVisibility::Hidden);
}

void UFTLoadingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UFTLoadingWidget::SetPercent(float Percent) const
{
	if(PB_LoadingBar != nullptr)
	{
		PB_LoadingBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UFTLoadingWidget::ReadyToStart()
{
	bReadyToStart = true;

	if (Image_0)
	{
		Image_0->SetBrushTintColor(FLinearColor::Black);
	}
	if (IMG_ReadyBlackBackground)
	{
		IMG_ReadyBlackBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if(PB_LoadingBar)
		PB_LoadingBar->SetVisibility(ESlateVisibility::Hidden);
	if(SizeBox_LoadingBar)
		SizeBox_LoadingBar->SetVisibility(ESlateVisibility::Hidden);
	if(TB_Text)
	{
		if (TB_Text_Opacity && !IsAnimationPlaying(TB_Text_Opacity))
			PlayAnimation(TB_Text_Opacity, 0, 0);
		TB_Text->SetText(FText::FromString(TEXT("클릭해서 계속")));
	}
	if(BTN_Click)
		BTN_Click->SetVisibility(ESlateVisibility::Visible);
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
	// if (TB_ObjectCounts)
	// {
	// 	const FText Template = FText::FromString(TEXT("{0} / {1}"));
	// 	const FText Display = FText::Format(
	// 		Template,
	// 		FText::AsNumber(CompleteNum),
	// 		FText::AsNumber(TotalNum));
	//
	// 	TB_ObjectCounts->SetVisibility(ESlateVisibility::Visible);
	// 	TB_ObjectCounts->SetText(Display);
	// }
	// if(TB_ObjectName)
	// {
	// 	TB_ObjectName->SetVisibility(ESlateVisibility::Visible);
	// 	TB_ObjectName->SetText(FText::FromString(ObjectName));
	// }
}

void UFTLoadingWidget::HandleButtonClicked()
{
	if (StoredCallback.IsSet())
	{
		StoredCallback.GetValue()();
	}
}
