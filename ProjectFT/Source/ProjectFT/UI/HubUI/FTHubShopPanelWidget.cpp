#include "FTHubShopPanelWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "FTItemTileListObject.h"
#include "ProjectFT/ViewModel/FTShopViewModel.h"
#include "Types/SlateEnums.h"

void UFTHubShopPanelWidget::InitializeShopPanel(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory)
{
	if (!ViewModel)
	{
		ViewModel = NewObject<UFTShopViewModel>(this);
<<<<<<< Updated upstream
	}

	ViewModel->OnChanged.RemoveDynamic(this, &UFTHubShopPanelWidget::RefreshFromViewModel);
	ViewModel->OnChanged.AddDynamic(this, &UFTHubShopPanelWidget::RefreshFromViewModel);
=======
		ViewModel->OnChanged.AddDynamic(this, &UFTHubShopPanelWidget::RefreshFromViewModel);
	}

>>>>>>> Stashed changes
	ViewModel->Initialize(InHubShop, InPlayerInventory);
	RefreshFromViewModel();
}

void UFTHubShopPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TV_ShopItems)
	{
		TV_ShopItems->SetSelectionMode(ESelectionMode::Single);
		TV_ShopItems->OnItemClicked().RemoveAll(this);
		TV_ShopItems->OnItemClicked().AddUObject(this, &UFTHubShopPanelWidget::HandleShopItemClicked);
		TV_ShopItems->OnItemSelectionChanged().RemoveAll(this);
		TV_ShopItems->OnItemSelectionChanged().AddUObject(this, &UFTHubShopPanelWidget::HandleShopItemSelectionChanged);
	}

	if (TV_PlayerItems)
	{
		TV_PlayerItems->SetSelectionMode(ESelectionMode::Single);
		TV_PlayerItems->OnItemClicked().RemoveAll(this);
		TV_PlayerItems->OnItemClicked().AddUObject(this, &UFTHubShopPanelWidget::HandlePlayerItemClicked);
		TV_PlayerItems->OnItemSelectionChanged().RemoveAll(this);
		TV_PlayerItems->OnItemSelectionChanged().AddUObject(this, &UFTHubShopPanelWidget::HandlePlayerItemSelectionChanged);
	}

	if (BTN_Buy)
	{
		BTN_Buy->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleBuyClicked);
		BTN_Buy->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleBuyClicked);
	}

	if (BTN_Sell)
	{
		BTN_Sell->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleSellClicked);
		BTN_Sell->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleSellClicked);
	}

	if (BTN_Refresh)
	{
		BTN_Refresh->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleRefreshClicked);
		BTN_Refresh->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleRefreshClicked);
	}

	RefreshFromViewModel();
}

void UFTHubShopPanelWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

<<<<<<< Updated upstream
	bRefreshingFromViewModel = true;
	PopulateTileItems(TV_ShopItems, ViewModel->GetShopItemObjects(), ViewModel->GetSelectedShopItemObject());
	PopulateTileItems(TV_PlayerItems, ViewModel->GetPlayerItemObjects(), ViewModel->GetSelectedPlayerItemObject());
	bRefreshingFromViewModel = false;
=======
	bUpdatingSelection = true;
	PopulateItems(TV_ShopItems, ViewModel->GetShopItemObjects(), ViewModel->GetSelectedShopItemObject());
	PopulateItems(TV_PlayerItems, ViewModel->GetPlayerItemObjects(), ViewModel->GetSelectedPlayerItemObject());
	bUpdatingSelection = false;
>>>>>>> Stashed changes

	if (TXT_SelectedItemName)
	{
		TXT_SelectedItemName->SetText(ViewModel->GetSelectedItemNameText());
	}

	if (TXT_SelectedItemDescription)
	{
		TXT_SelectedItemDescription->SetText(ViewModel->GetSelectedItemDescriptionText());
	}

	if (TXT_SelectedItemPrice)
	{
		TXT_SelectedItemPrice->SetText(ViewModel->GetSelectedItemPriceText());
	}

	if (TXT_SelectedItemCount)
	{
		TXT_SelectedItemCount->SetText(ViewModel->GetSelectedItemCountText());
	}

	if (TXT_SelectedItemState)
	{
		TXT_SelectedItemState->SetText(ViewModel->GetSelectedItemStateText());
	}

	if (BTN_Buy)
	{
		BTN_Buy->SetIsEnabled(ViewModel->CanBuySelectedItem());
	}

	if (BTN_Sell)
	{
		BTN_Sell->SetIsEnabled(ViewModel->CanSellSelectedItem());
	}
}

<<<<<<< Updated upstream
void UFTHubShopPanelWidget::PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem)
=======
void UFTHubShopPanelWidget::PopulateItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem)
>>>>>>> Stashed changes
{
	if (!TileView)
	{
		return;
	}

	TileView->ClearListItems();
	for (UObject* Item : Items)
	{
		TileView->AddItem(Item);
	}

	if (SelectedItem)
	{
		TileView->SetItemSelection(SelectedItem, true);
	}

	TileView->RequestRefresh();
}

void UFTHubShopPanelWidget::HandleShopItemClicked(UObject* Item)
{
<<<<<<< Updated upstream
	if (bRefreshingFromViewModel || !ViewModel)
=======
	if (bUpdatingSelection || !ViewModel)
>>>>>>> Stashed changes
	{
		return;
	}

	if (TV_PlayerItems)
	{
		TV_PlayerItems->ClearSelection();
	}

	ViewModel->SelectShopItemObject(Item);
}

void UFTHubShopPanelWidget::HandlePlayerItemClicked(UObject* Item)
{
<<<<<<< Updated upstream
	if (bRefreshingFromViewModel || !ViewModel)
=======
	if (bUpdatingSelection || !ViewModel)
>>>>>>> Stashed changes
	{
		return;
	}

	if (TV_ShopItems)
	{
		TV_ShopItems->ClearSelection();
	}

	ViewModel->SelectPlayerItemObject(Item);
}

void UFTHubShopPanelWidget::HandleShopItemSelectionChanged(UObject* Item)
{
	if (Item)
	{
		HandleShopItemClicked(Item);
	}
}

void UFTHubShopPanelWidget::HandlePlayerItemSelectionChanged(UObject* Item)
{
	if (Item)
	{
		HandlePlayerItemClicked(Item);
	}
}

void UFTHubShopPanelWidget::HandleBuyClicked()
{
	if (ViewModel)
	{
		ViewModel->BuySelectedItem();
	}
}

void UFTHubShopPanelWidget::HandleSellClicked()
{
	if (ViewModel)
	{
		ViewModel->SellSelectedItem();
	}
}

void UFTHubShopPanelWidget::HandleRefreshClicked()
{
	if (ViewModel)
	{
<<<<<<< Updated upstream
		ViewModel->RefreshShopStock();
=======
		ViewModel->RefreshShop();
>>>>>>> Stashed changes
	}
}
