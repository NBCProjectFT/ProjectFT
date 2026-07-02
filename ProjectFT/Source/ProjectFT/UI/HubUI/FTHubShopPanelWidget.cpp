#include "FTHubShopPanelWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "FTItemTileListObject.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubShop.h"
#include "Types/SlateEnums.h"

void UFTHubShopPanelWidget::InitializeShopPanel(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory)
{
	HubShop = InHubShop;
	PlayerInventory = InPlayerInventory;
	SelectedShopItem = nullptr;
	SelectedPlayerItem = nullptr;
	SelectedSource = EShopSelectionSourceType::None;
	RefreshAllItems();
	UpdateSelectedItemDetails();
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

	RefreshAllItems();
	UpdateSelectedItemDetails();
}

void UFTHubShopPanelWidget::RefreshShopItems()
{
	if (!TV_ShopItems)
	{
		return;
	}

	const FName SelectedItemID = SelectedShopItem
		? SelectedShopItem->GetItemID()
		: NAME_None;

	SelectedShopItem = nullptr;
	TV_ShopItems->ClearListItems();

	if (!HubShop)
	{
		return;
	}

	TArray<FTShopItemStruct> ShopItems;
	HubShop->GetShopItems(ShopItems);

	for (const FTShopItemStruct& ShopItem : ShopItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeShopItem(ShopItem, !HubShop->IsShopItemUnlocked(ShopItem.ItemID));
		TV_ShopItems->AddItem(ItemObject);

		if (ShopItem.ItemID == SelectedItemID)
		{
			SelectedShopItem = ItemObject;
			TV_ShopItems->SetItemSelection(ItemObject, true);
		}
	}
}

void UFTHubShopPanelWidget::RefreshPlayerItems()
{
	if (!TV_PlayerItems)
	{
		return;
	}

	const FName SelectedItemID = SelectedPlayerItem
		? SelectedPlayerItem->GetItemID()
		: NAME_None;

	SelectedPlayerItem = nullptr;
	TV_PlayerItems->ClearListItems();

	if (!PlayerInventory)
	{
		return;
	}

	for (const FFTInventoryItem& InventoryItem : PlayerInventory->GetItems())
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		const int32 SellPrice = HubShop
			? HubShop->GetShopSellPrice(InventoryItem.ItemId)
			: 0;
		ItemObject->InitializeItem(InventoryItem.ItemId, InventoryItem.Quantity, SellPrice);
		TV_PlayerItems->AddItem(ItemObject);

		if (InventoryItem.ItemId == SelectedItemID)
		{
			SelectedPlayerItem = ItemObject;
			TV_PlayerItems->SetItemSelection(ItemObject, true);
		}
	}
}

void UFTHubShopPanelWidget::RefreshAllItems()
{
	RefreshShopItems();
	RefreshPlayerItems();
}

void UFTHubShopPanelWidget::UpdateSelectedItemDetails()
{
	const UFTItemTileListObject* SelectedItem = nullptr;
	if (SelectedSource == EShopSelectionSourceType::Shop)
	{
		SelectedItem = SelectedShopItem;
	}
	else if (SelectedSource == EShopSelectionSourceType::Player)
	{
		SelectedItem = SelectedPlayerItem;
	}

	const bool bHasSelection = SelectedItem != nullptr;
	const FName ItemID = bHasSelection ? SelectedItem->GetItemID() : NAME_None;
	const bool bCanBuy = SelectedSource == EShopSelectionSourceType::Shop && HubShop && HubShop->CanBuyItem(ItemID, PlayerInventory);
	const bool bCanSell = SelectedSource == EShopSelectionSourceType::Player && HubShop && HubShop->CanSellItemToShop(ItemID, 1, PlayerInventory);

	if (TXT_SelectedItemName)
	{
		TXT_SelectedItemName->SetText(bHasSelection
			? SelectedItem->GetDisplayName()
			: FText::FromString(TEXT("Select Item")));
	}

	if (TXT_SelectedItemDescription)
	{
		TXT_SelectedItemDescription->SetText(bHasSelection
			? SelectedItem->GetDescription()
			: FText::GetEmpty());
	}

	if (TXT_SelectedItemPrice)
	{
		TXT_SelectedItemPrice->SetText(bHasSelection
			? FText::FromString(FString::Printf(TEXT("Price: %d"), SelectedItem->GetPrice()))
			: FText::GetEmpty());
	}

	if (TXT_SelectedItemCount)
	{
		TXT_SelectedItemCount->SetText(bHasSelection
			? FText::FromString(FString::Printf(TEXT("Count: %d"), SelectedItem->GetCount()))
			: FText::GetEmpty());
	}

	if (TXT_SelectedItemState)
	{
		TXT_SelectedItemState->SetText(bHasSelection
			? (bCanBuy || bCanSell ? FText::FromString(TEXT("거래 가능")) : FText::FromString(TEXT("거래 불가")))
			: FText::GetEmpty());
	}

	if (BTN_Buy)
	{
		BTN_Buy->SetIsEnabled(bCanBuy);
	}

	if (BTN_Sell)
	{
		BTN_Sell->SetIsEnabled(bCanSell);
	}
}

void UFTHubShopPanelWidget::ClearTileChecks(UTileView* TileView)
{
	if (!TileView)
	{
		return;
	}

	const TArray<UObject*> ListItems = TileView->GetListItems();
	for (UObject* ListItem : ListItems)
	{
		if (UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ListItem))
		{
			TileObject->SetChecked(false);
		}
	}

	TileView->RequestRefresh();
}

void UFTHubShopPanelWidget::HandleShopItemClicked(UObject* Item)
{
	if (bUpdatingSelection)
	{
		return;
	}

	SelectedShopItem = Cast<UFTItemTileListObject>(Item);
	SelectedPlayerItem = nullptr;
	SelectedSource = EShopSelectionSourceType::Shop;

	bUpdatingSelection = true;

	ClearTileChecks(TV_ShopItems);
	ClearTileChecks(TV_PlayerItems);

	if (SelectedShopItem)
	{
		SelectedShopItem->SetChecked(true);
	}

	if (TV_PlayerItems)
	{
		TV_PlayerItems->ClearSelection();
	}

	if (TV_ShopItems && SelectedShopItem)
	{
		TV_ShopItems->SetItemSelection(SelectedShopItem, true);
		TV_ShopItems->RequestRefresh();
	}

	bUpdatingSelection = false;

	UpdateSelectedItemDetails();
}

void UFTHubShopPanelWidget::HandlePlayerItemClicked(UObject* Item)
{
	if (bUpdatingSelection)
	{
		return;
	}

	SelectedPlayerItem = Cast<UFTItemTileListObject>(Item);
	SelectedShopItem = nullptr;
	SelectedSource = EShopSelectionSourceType::Player;

	bUpdatingSelection = true;

	ClearTileChecks(TV_ShopItems);
	ClearTileChecks(TV_PlayerItems);

	if (SelectedPlayerItem)
	{
		SelectedPlayerItem->SetChecked(true);
	}

	if (TV_ShopItems)
	{
		TV_ShopItems->ClearSelection();
	}

	if (TV_PlayerItems && SelectedPlayerItem)
	{
		TV_PlayerItems->SetItemSelection(SelectedPlayerItem, true);
		TV_PlayerItems->RequestRefresh();
	}

	bUpdatingSelection = false;

	UpdateSelectedItemDetails();
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
	if (!HubShop || !SelectedShopItem)
	{
		return;
	}

	if (HubShop->BuyItem(SelectedShopItem->GetItemID(), PlayerInventory))
	{
		RefreshAllItems();
		UpdateSelectedItemDetails();
	}
}

void UFTHubShopPanelWidget::HandleSellClicked()
{
	if (!HubShop || !SelectedPlayerItem)
	{
		return;
	}

	if (HubShop->SellItemToShop(SelectedPlayerItem->GetItemID(), 1, PlayerInventory))
	{
		SelectedPlayerItem = nullptr;
		SelectedSource = EShopSelectionSourceType::None;
		RefreshAllItems();
		UpdateSelectedItemDetails();
	}
}

void UFTHubShopPanelWidget::HandleRefreshClicked()
{
	if (!HubShop)
	{
		return;
	}

	HubShop->RefreshShopItems();
	SelectedShopItem = nullptr;
	SelectedSource = EShopSelectionSourceType::None;
	RefreshAllItems();
	UpdateSelectedItemDetails();
}
