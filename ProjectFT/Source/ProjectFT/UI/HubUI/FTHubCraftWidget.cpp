#include "FTHubCraftWidget.h"

#include "InputCoreTypes.h"
#include "ProjectFT/Core/FTCraftingSubsystem.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTCraftingViewModel.h"

void UFTHubCraftWidget::InitializeCraftWidget(
	UFTInventoryComponent* InPlayerInventory,
	UFTInventoryComponent* InStorageInventory,
	UFTCraftingViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTHubCraftWidget::RefreshFromViewModel);
	}

	ViewModel = InViewModel ? InViewModel : NewObject<UFTCraftingViewModel>(this);
	if (ViewModel)
	{
		UFTCraftingSubsystem* CraftingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UFTCraftingSubsystem>()
			: nullptr;

		ViewModel->Initialize(CraftingSubsystem, InPlayerInventory, InStorageInventory);
		ViewModel->OnChanged.RemoveDynamic(this, &UFTHubCraftWidget::RefreshFromViewModel);
		ViewModel->OnChanged.AddDynamic(this, &UFTHubCraftWidget::RefreshFromViewModel);
	}
}

void UFTHubCraftWidget::CloseCraft()
{
	if (UFTUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTUIManagerSubsystem>()
		: nullptr)
	{
		UIManager->HideCrafting();
	}
}

void UFTHubCraftWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshFromViewModel();
}

FReply UFTHubCraftWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::E)
	{
		CloseCraft();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFTHubCraftWidget::RefreshFromViewModel()
{
	if (ViewModel)
	{
		BP_OnCraftingViewModelChanged(ViewModel);
	}
}
