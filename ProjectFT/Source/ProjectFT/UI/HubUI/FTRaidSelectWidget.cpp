#include "FTRaidSelectWidget.h"

#include "InputCoreTypes.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTRaidSelectViewModel.h"

void UFTRaidSelectWidget::InitializeRaidSelect(UFTRaidSelectViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
	}

	ViewModel = InViewModel;
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
		ViewModel->OnChanged.AddDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
	}

	RefreshFromViewModel();
}

void UFTRaidSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshFromViewModel();
}

void UFTRaidSelectWidget::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTRaidSelectWidget::RefreshFromViewModel);
	}
	Super::NativeDestruct();
}

FReply UFTRaidSelectWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::E)
	{
		CloseRaidSelect();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFTRaidSelectWidget::CloseRaidSelect()
{
	if (UFTUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTUIManagerSubsystem>()
		: nullptr)
	{
		UIManager->HideRaidSelect();
	}
}

void UFTRaidSelectWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

	BP_OnRaidViewModelChanged(ViewModel);
}
