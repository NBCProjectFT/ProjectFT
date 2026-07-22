#include "FTHubShopPanelWidget.h"

#include "ProjectFT/ViewModel/FTShopViewModel.h"

void UFTHubShopPanelWidget::InitializeShopPanel(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	if (!ViewModel)
	{
		ViewModel = NewObject<UFTShopViewModel>(this);
	}

	ViewModel->OnChanged.RemoveDynamic(this, &UFTHubShopPanelWidget::RefreshFromViewModel);
	ViewModel->OnChanged.AddDynamic(this, &UFTHubShopPanelWidget::RefreshFromViewModel);
	ViewModel->Initialize(InShopSubsystem, InPlayerInventory);
}

void UFTHubShopPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshFromViewModel();
}

void UFTHubShopPanelWidget::RefreshFromViewModel()
{
	if (ViewModel)
	{
		BP_OnShopViewModelChanged(ViewModel);
	}
}
