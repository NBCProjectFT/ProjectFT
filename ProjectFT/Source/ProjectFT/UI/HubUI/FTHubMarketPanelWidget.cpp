#include "FTHubMarketPanelWidget.h"

#include "ProjectFT/ViewModel/FTMarketViewModel.h"

void UFTHubMarketPanelWidget::InitializeMarketPanel(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	if (!ViewModel)
	{
		ViewModel = NewObject<UFTMarketViewModel>(this);
	}

	ViewModel->OnChanged.RemoveDynamic(this, &UFTHubMarketPanelWidget::RefreshFromViewModel);
	ViewModel->OnChanged.AddDynamic(this, &UFTHubMarketPanelWidget::RefreshFromViewModel);

	ViewModel->Initialize(InShopSubsystem, InPlayerInventory);
}

void UFTHubMarketPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTHubMarketPanelWidget::RefreshFromViewModel);
		ViewModel->OnChanged.AddDynamic(this, &UFTHubMarketPanelWidget::RefreshFromViewModel);
	}
	RefreshFromViewModel();
}

void UFTHubMarketPanelWidget::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->OnChanged.RemoveDynamic(this, &UFTHubMarketPanelWidget::RefreshFromViewModel);
	}

	Super::NativeDestruct();
}

void UFTHubMarketPanelWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

	BP_OnMarketViewModelChanged(ViewModel);
}
