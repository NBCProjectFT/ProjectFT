#include "FTHubMarketPanelWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Engine/Texture2D.h"
#include "FTTradePostListObject.h"
#include "ProjectFT/ViewModel/FTMarketViewModel.h"
#include "Types/SlateEnums.h"

void UFTHubMarketPanelWidget::InitializeMarketPanel(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	if (!ViewModel)
	{
		ViewModel = NewObject<UFTMarketViewModel>(this);
		ViewModel->OnChanged.AddDynamic(this, &UFTHubMarketPanelWidget::RefreshFromViewModel);
	}

	ViewModel->Initialize(InShopSubsystem, InPlayerInventory);
	RefreshFromViewModel();
}

void UFTHubMarketPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LV_TradePosts)
	{
		LV_TradePosts->SetSelectionMode(ESelectionMode::Single);
		LV_TradePosts->OnItemClicked().RemoveAll(this);
		LV_TradePosts->OnItemClicked().AddUObject(this, &UFTHubMarketPanelWidget::HandleTradePostClicked);
	}

	if (BTN_BuyRequestsTab)
	{
		BTN_BuyRequestsTab->OnClicked.RemoveDynamic(this, &UFTHubMarketPanelWidget::HandleBuyRequestsTabClicked);
		BTN_BuyRequestsTab->OnClicked.AddDynamic(this, &UFTHubMarketPanelWidget::HandleBuyRequestsTabClicked);
	}

	if (BTN_SellOffersTab)
	{
		BTN_SellOffersTab->OnClicked.RemoveDynamic(this, &UFTHubMarketPanelWidget::HandleSellOffersTabClicked);
		BTN_SellOffersTab->OnClicked.AddDynamic(this, &UFTHubMarketPanelWidget::HandleSellOffersTabClicked);
	}

	if (BTN_TradeAction)
	{
		BTN_TradeAction->OnClicked.RemoveDynamic(this, &UFTHubMarketPanelWidget::HandleTradeClicked);
		BTN_TradeAction->OnClicked.AddDynamic(this, &UFTHubMarketPanelWidget::HandleTradeClicked);
	}
	else if (BTN_Trade)
	{
		BTN_Trade->OnClicked.RemoveDynamic(this, &UFTHubMarketPanelWidget::HandleTradeClicked);
		BTN_Trade->OnClicked.AddDynamic(this, &UFTHubMarketPanelWidget::HandleTradeClicked);
	}

	RefreshFromViewModel();
}

void UFTHubMarketPanelWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

	bRefreshingFromViewModel = true;
	PopulateListItems(LV_TradePosts, ViewModel->GetTradePostObjects(), ViewModel->GetSelectedPostObject());
	PopulateTileItems(TV_SelectedPostItems, ViewModel->GetSelectedPostItemObjects());
	bRefreshingFromViewModel = false;

	if (TXT_SelectedPostTitle)
	{
		TXT_SelectedPostTitle->SetText(ViewModel->GetSelectedPostTitleText());
	}

	if (TXT_SelectedPostDescription)
	{
		TXT_SelectedPostDescription->SetText(ViewModel->GetSelectedPostDescriptionText());
	}

	if (TXT_SelectedPostItem)
	{
		TXT_SelectedPostItem->SetText(ViewModel->GetSelectedPostItemText());
	}

	if (TXT_SelectedPostPrice)
	{
		TXT_SelectedPostPrice->SetText(ViewModel->GetSelectedPostPriceText());
	}

	if (TXT_SelectedItemName)
	{
		TXT_SelectedItemName->SetText(ViewModel->GetSelectedItemNameText());
	}

	if (TXT_SelectedItemTag)
	{
		TXT_SelectedItemTag->SetText(ViewModel->GetSelectedItemTagText());
	}

	if (TXT_SelectedItemOwnedCount)
	{
		TXT_SelectedItemOwnedCount->SetText(ViewModel->GetSelectedItemOwnedCountText());
	}

	if (TXT_SelectedItemDescription)
	{
		TXT_SelectedItemDescription->SetText(ViewModel->GetSelectedItemDescriptionText());
	}

	if (TXT_SelectedItemPrice)
	{
		TXT_SelectedItemPrice->SetText(ViewModel->GetSelectedPostPriceText());
	}

	if (TXT_TradeAction)
	{
		TXT_TradeAction->SetText(ViewModel->GetTradeActionText());
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

	if (BTN_TradeAction)
	{
		BTN_TradeAction->SetIsEnabled(ViewModel->CanTradeSelectedPost());
	}
	else if (BTN_Trade)
	{
		BTN_Trade->SetIsEnabled(ViewModel->CanTradeSelectedPost());
	}
}

void UFTHubMarketPanelWidget::PopulateListItems(UListView* ListView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem)
{
	if (!ListView)
	{
		return;
	}

	ListView->ClearListItems();
	for (UObject* Item : Items)
	{
		ListView->AddItem(Item);
	}

	if (SelectedItem)
	{
		ListView->SetItemSelection(SelectedItem, true);
	}

	ListView->RequestRefresh();
}

void UFTHubMarketPanelWidget::PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items)
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

	TileView->RequestRefresh();
}

void UFTHubMarketPanelWidget::HandleTradePostClicked(UObject* Item)
{
	if (bRefreshingFromViewModel || !ViewModel)
	{
		return;
	}

	ViewModel->SelectTradePostObject(Item);
}

void UFTHubMarketPanelWidget::HandleBuyRequestsTabClicked()
{
	if (ViewModel)
	{
		ViewModel->SetBuyRequestMode(true);
	}
}

void UFTHubMarketPanelWidget::HandleSellOffersTabClicked()
{
	if (ViewModel)
	{
		ViewModel->SetBuyRequestMode(false);
	}
}

void UFTHubMarketPanelWidget::HandleTradeClicked()
{
	if (ViewModel)
	{
		ViewModel->TradeSelectedPost();
	}
}
