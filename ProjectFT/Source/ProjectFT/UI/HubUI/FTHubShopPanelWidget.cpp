#include "FTHubShopPanelWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Engine/Texture2D.h"
#include "FTItemTileListObject.h"
#include "ProjectFT/ViewModel/FTShopViewModel.h"
#include "Types/SlateEnums.h"

void UFTHubShopPanelWidget::InitializeShopPanel(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	if (!ViewModel)
	{
		ViewModel = NewObject<UFTShopViewModel>(this);
	}

	ViewModel->OnChanged.RemoveDynamic(this, &UFTHubShopPanelWidget::RefreshFromViewModel);
	ViewModel->OnChanged.AddDynamic(this, &UFTHubShopPanelWidget::RefreshFromViewModel);
	ViewModel->Initialize(InShopSubsystem, InPlayerInventory);
	RefreshFromViewModel();
}

void UFTHubShopPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UTileView* PrimaryTileView = GetPrimaryTileView())
	{
		PrimaryTileView->SetSelectionMode(ESelectionMode::Single);
		PrimaryTileView->OnItemClicked().RemoveAll(this);
		PrimaryTileView->OnItemClicked().AddUObject(this, &UFTHubShopPanelWidget::HandleItemClicked);
		PrimaryTileView->OnItemSelectionChanged().RemoveAll(this);
		PrimaryTileView->OnItemSelectionChanged().AddUObject(this, &UFTHubShopPanelWidget::HandleItemSelectionChanged);
	}

	if (TV_PlayerItems)
	{
		TV_PlayerItems->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (BTN_BuyMode)
	{
		BTN_BuyMode->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleBuyModeClicked);
		BTN_BuyMode->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleBuyModeClicked);
	}

	if (BTN_Buy)
	{
		BTN_Buy->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleBuyModeClicked);
		BTN_Buy->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleBuyModeClicked);
	}

	if (BTN_SellMode)
	{
		BTN_SellMode->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleSellModeClicked);
		BTN_SellMode->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleSellModeClicked);
	}

	if (BTN_Sell)
	{
		BTN_Sell->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleSellModeClicked);
		BTN_Sell->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleSellModeClicked);
	}

	if (BTN_QuantityMinus)
	{
		BTN_QuantityMinus->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleQuantityMinusClicked);
		BTN_QuantityMinus->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleQuantityMinusClicked);
	}

	if (BTN_QuantityPlus)
	{
		BTN_QuantityPlus->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleQuantityPlusClicked);
		BTN_QuantityPlus->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleQuantityPlusClicked);
	}

	if (BTN_TradeAction)
	{
		BTN_TradeAction->OnClicked.RemoveDynamic(this, &UFTHubShopPanelWidget::HandleTradeActionClicked);
		BTN_TradeAction->OnClicked.AddDynamic(this, &UFTHubShopPanelWidget::HandleTradeActionClicked);
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

	bRefreshingFromViewModel = true;
	PopulateTileItems(GetPrimaryTileView(), ViewModel->GetCurrentItemObjects(), ViewModel->GetSelectedCurrentItemObject());
	bRefreshingFromViewModel = false;

	const FText SelectedItemName = ViewModel->GetSelectedItemNameText();
	if (TXT_SelectedItemName)
	{
		TXT_SelectedItemName->SetText(SelectedItemName);
	}

	if (TXT_ItemName)
	{
		TXT_ItemName->SetText(SelectedItemName);
	}

	const FText SelectedItemTag = ViewModel->GetSelectedItemTagText();
	if (TXT_SelectedItemTag)
	{
		TXT_SelectedItemTag->SetText(SelectedItemTag);
	}

	if (TXT_Tag)
	{
		TXT_Tag->SetText(SelectedItemTag);
	}

	if (TXT_SelectedItemDescription)
	{
		TXT_SelectedItemDescription->SetText(ViewModel->GetSelectedItemDescriptionText());
	}

	const FText OwnedCountText = ViewModel->GetSelectedItemOwnedCountText();
	if (TXT_SelectedItemOwnedCount)
	{
		TXT_SelectedItemOwnedCount->SetText(OwnedCountText);
	}

	if (TXT_ItemCount)
	{
		TXT_ItemCount->SetText(OwnedCountText);
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

	if (TXT_TradeQuantity)
	{
		TXT_TradeQuantity->SetText(ViewModel->GetTradeQuantityText());
	}

	if (TXT_TotalPrice)
	{
		TXT_TotalPrice->SetText(ViewModel->GetTradeTotalPriceText());
	}

	if (TXT_TradeAction)
	{
		TXT_TradeAction->SetText(ViewModel->GetTradeActionText());
	}

	if (BTN_TradeAction)
	{
		BTN_TradeAction->SetIsEnabled(ViewModel->CanExecuteTradeAction());
	}

	if (IMG_SelectedItemIcon)
	{
		if (UTexture2D* IconTexture = ViewModel->GetSelectedItemIcon().LoadSynchronous())
		{
			IMG_SelectedItemIcon->SetBrushFromTexture(IconTexture, true);
			IMG_SelectedItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IMG_SelectedItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

UTileView* UFTHubShopPanelWidget::GetPrimaryTileView() const
{
	return TV_Items ? TV_Items : TV_ShopItems;
}

void UFTHubShopPanelWidget::PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem)
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

void UFTHubShopPanelWidget::HandleItemClicked(UObject* Item)
{
	if (bRefreshingFromViewModel || !ViewModel)
	{
		return;
	}

	ViewModel->SelectCurrentItemObject(Item);
}

void UFTHubShopPanelWidget::HandleItemSelectionChanged(UObject* Item)
{
	if (Item)
	{
		HandleItemClicked(Item);
	}
}

void UFTHubShopPanelWidget::HandleBuyModeClicked()
{
	if (ViewModel)
	{
		ViewModel->SetBuyMode();
	}
}

void UFTHubShopPanelWidget::HandleSellModeClicked()
{
	if (ViewModel)
	{
		ViewModel->SetSellMode();
	}
}

void UFTHubShopPanelWidget::HandleQuantityMinusClicked()
{
	if (ViewModel)
	{
		ViewModel->DecreaseTradeQuantity();
	}
}

void UFTHubShopPanelWidget::HandleQuantityPlusClicked()
{
	if (ViewModel)
	{
		ViewModel->IncreaseTradeQuantity();
	}
}

void UFTHubShopPanelWidget::HandleTradeActionClicked()
{
	if (ViewModel)
	{
		ViewModel->ExecuteTradeAction();
	}
}

void UFTHubShopPanelWidget::HandleRefreshClicked()
{
	if (ViewModel)
	{
		ViewModel->RefreshShop();
	}
}
