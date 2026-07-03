#include "FTShopViewModel.h"

#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Hub/FTHubShop.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"

void UFTShopViewModel::Initialize(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventoryDelegate();

	HubShop = InHubShop;
	PlayerInventory = InPlayerInventory;
	ClearSelection();

	BindInventoryDelegate();
	RefreshAll();
}

const TArray<TObjectPtr<UObject>>& UFTShopViewModel::GetShopItemObjects() const
{
	return ShopItemObjects;
}

const TArray<TObjectPtr<UObject>>& UFTShopViewModel::GetPlayerItemObjects() const
{
	return PlayerItemObjects;
}

UFTItemTileListObject* UFTShopViewModel::GetSelectedShopItemObject() const
{
	return SelectedShopItemObject;
}

UFTItemTileListObject* UFTShopViewModel::GetSelectedPlayerItemObject() const
{
	return SelectedPlayerItemObject;
}

FText UFTShopViewModel::GetSelectedItemNameText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItemObject();
	return SelectedItem ? SelectedItem->GetDisplayName() : FText::FromString(TEXT("Select Item"));
}

FText UFTShopViewModel::GetSelectedItemDescriptionText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItemObject();
	return SelectedItem ? SelectedItem->GetDescription() : FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemPriceText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItemObject();
	return SelectedItem
		? FText::FromString(FString::Printf(TEXT("Price: %d"), SelectedItem->GetPrice()))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemCountText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItemObject();
	return SelectedItem
		? FText::FromString(FString::Printf(TEXT("Count: %d"), SelectedItem->GetCount()))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemStateText() const
{
	if (!GetSelectedItemObject())
	{
		return FText::GetEmpty();
	}

	return CanBuySelectedItem() || CanSellSelectedItem()
		? FText::FromString(TEXT("Trade Available"))
		: FText::FromString(TEXT("Trade Unavailable"));
}

bool UFTShopViewModel::CanBuySelectedItem() const
{
	return SelectedSource == EShopSelectionSourceType::Shop
		&& HubShop
		&& SelectedShopItemObject
		&& HubShop->CanBuyItem(SelectedShopItemObject->GetItemID(), PlayerInventory);
}

bool UFTShopViewModel::CanSellSelectedItem() const
{
	return SelectedSource == EShopSelectionSourceType::Player
		&& HubShop
		&& SelectedPlayerItemObject
		&& HubShop->CanSellItemToShop(SelectedPlayerItemObject->GetItemID(), 1, PlayerInventory);
}

void UFTShopViewModel::RefreshAll()
{
	const FName PreviousShopItemID = SelectedShopItemObject ? SelectedShopItemObject->GetItemID() : NAME_None;
	const FName PreviousPlayerItemID = SelectedPlayerItemObject ? SelectedPlayerItemObject->GetItemID() : NAME_None;
	const EShopSelectionSourceType PreviousSource = SelectedSource;

	RefreshShopItems();
	RefreshPlayerItems();
	RestoreSelection(PreviousShopItemID, PreviousPlayerItemID, PreviousSource);
	NotifyChanged();
}

void UFTShopViewModel::SelectShopItemObject(UObject* ItemObject)
{
	SelectedShopItemObject = Cast<UFTItemTileListObject>(ItemObject);
	SelectedPlayerItemObject = nullptr;
	SelectedSource = SelectedShopItemObject ? EShopSelectionSourceType::Shop : EShopSelectionSourceType::None;
	ClearItemChecks();

	if (SelectedShopItemObject)
	{
		SelectedShopItemObject->SetChecked(true);
	}

	NotifyChanged();
}

void UFTShopViewModel::SelectPlayerItemObject(UObject* ItemObject)
{
	SelectedPlayerItemObject = Cast<UFTItemTileListObject>(ItemObject);
	SelectedShopItemObject = nullptr;
	SelectedSource = SelectedPlayerItemObject ? EShopSelectionSourceType::Player : EShopSelectionSourceType::None;
	ClearItemChecks();

	if (SelectedPlayerItemObject)
	{
		SelectedPlayerItemObject->SetChecked(true);
	}

	NotifyChanged();
}

bool UFTShopViewModel::BuySelectedItem()
{
	if (!HubShop || !SelectedShopItemObject)
	{
		return false;
	}

	if (!HubShop->BuyItem(SelectedShopItemObject->GetItemID(), PlayerInventory))
	{
		return false;
	}

	RefreshAll();
	return true;
}

bool UFTShopViewModel::SellSelectedItem()
{
	if (!HubShop || !SelectedPlayerItemObject)
	{
		return false;
	}

	if (!HubShop->SellItemToShop(SelectedPlayerItemObject->GetItemID(), 1, PlayerInventory))
	{
		return false;
	}

	ClearSelection();
	RefreshAll();
	return true;
}

void UFTShopViewModel::RefreshShopStock()
{
	if (!HubShop)
	{
		return;
	}

	HubShop->RefreshShopItems();
	ClearSelection();
	RefreshAll();
}

void UFTShopViewModel::HandleInventoryChanged()
{
	RefreshAll();
}

void UFTShopViewModel::RefreshShopItems()
{
	ShopItemObjects.Reset();

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
		ShopItemObjects.Add(ItemObject);
	}
}

void UFTShopViewModel::RefreshPlayerItems()
{
	PlayerItemObjects.Reset();

	if (!PlayerInventory)
	{
		return;
	}

	for (const FFTInventoryItem& InventoryItem : PlayerInventory->GetItems())
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		const int32 SellPrice = HubShop ? HubShop->GetShopSellPrice(InventoryItem.ItemId) : 0;
		ItemObject->InitializeItem(InventoryItem.ItemId, InventoryItem.Quantity, SellPrice);
		PlayerItemObjects.Add(ItemObject);
	}
}

void UFTShopViewModel::RestoreSelection(
	const FName PreviousShopItemID,
	const FName PreviousPlayerItemID,
	const EShopSelectionSourceType PreviousSource
)
{
	SelectedShopItemObject = nullptr;
	SelectedPlayerItemObject = nullptr;
	SelectedSource = EShopSelectionSourceType::None;

	if (PreviousSource == EShopSelectionSourceType::Shop && !PreviousShopItemID.IsNone())
	{
		for (UObject* ItemObject : ShopItemObjects)
		{
			UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject);
			if (TileObject && TileObject->GetItemID() == PreviousShopItemID)
			{
				SelectedShopItemObject = TileObject;
				SelectedSource = EShopSelectionSourceType::Shop;
				TileObject->SetChecked(true);
				return;
			}
		}
	}

	if (PreviousSource == EShopSelectionSourceType::Player && !PreviousPlayerItemID.IsNone())
	{
		for (UObject* ItemObject : PlayerItemObjects)
		{
			UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject);
			if (TileObject && TileObject->GetItemID() == PreviousPlayerItemID)
			{
				SelectedPlayerItemObject = TileObject;
				SelectedSource = EShopSelectionSourceType::Player;
				TileObject->SetChecked(true);
				return;
			}
		}
	}
}

void UFTShopViewModel::ClearSelection()
{
	SelectedShopItemObject = nullptr;
	SelectedPlayerItemObject = nullptr;
	SelectedSource = EShopSelectionSourceType::None;
	ClearItemChecks();
}

void UFTShopViewModel::ClearItemChecks()
{
	for (UObject* ItemObject : ShopItemObjects)
	{
		if (UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject))
		{
			TileObject->SetChecked(false);
		}
	}

	for (UObject* ItemObject : PlayerItemObjects)
	{
		if (UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject))
		{
			TileObject->SetChecked(false);
		}
	}
}

void UFTShopViewModel::BindInventoryDelegate()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTShopViewModel::HandleInventoryChanged);
		PlayerInventory->OnInventoryChanged.AddDynamic(this, &UFTShopViewModel::HandleInventoryChanged);
	}
}

void UFTShopViewModel::UnbindInventoryDelegate()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTShopViewModel::HandleInventoryChanged);
	}
}

UFTItemTileListObject* UFTShopViewModel::GetSelectedItemObject() const
{
	if (SelectedSource == EShopSelectionSourceType::Shop)
	{
		return SelectedShopItemObject;
	}

	if (SelectedSource == EShopSelectionSourceType::Player)
	{
		return SelectedPlayerItemObject;
	}

	return nullptr;
}

void UFTShopViewModel::NotifyChanged()
{
	OnChanged.Broadcast();
}
