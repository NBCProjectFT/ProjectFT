#include "FTHubStorageWidget.h"

#include "InputCoreTypes.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/ViewModel/FTHubStorageViewModel.h"

void UFTHubStorageWidget::InitializeStorageWidget(
	AFTHubStorage* InHubStorage,
	UFTInventoryComponent* InPlayerInventory,
	UFTHubStorageViewModel* InViewModel)
{
	HubStorage = InHubStorage;

	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTHubStorageWidget::RefreshFromViewModel);
	}

	ViewModel = InViewModel ? InViewModel : NewObject<UFTHubStorageViewModel>(this);
	if (ViewModel)
	{
		// Initialize broadcasts once, so bind afterward and let NativeConstruct perform
		// the single initial Blueprint presentation refresh when the widget is shown.
		ViewModel->Initialize(HubStorage, InPlayerInventory);
		ViewModel->OnChanged.RemoveDynamic(this, &UFTHubStorageWidget::RefreshFromViewModel);
		ViewModel->OnChanged.AddDynamic(this, &UFTHubStorageWidget::RefreshFromViewModel);
	}
}

void UFTHubStorageWidget::CloseStorage()
{
	if (HubStorage)
	{
		HubStorage->CloseStorageWidget();
	}
}

void UFTHubStorageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshFromViewModel();
}

FReply UFTHubStorageWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::E)
	{
		CloseStorage();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFTHubStorageWidget::RefreshFromViewModel()
{
	if (ViewModel)
	{
		BP_OnStorageViewModelChanged(ViewModel);
	}
}
