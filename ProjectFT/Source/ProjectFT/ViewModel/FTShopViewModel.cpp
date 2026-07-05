#include "FTShopViewModel.h"

#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Struct/FTShopItemStruct.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"

void UFTShopViewModel::Initialize(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventoryDelegate();

	ShopSubsystem = InShopSubsystem;
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
	return SelectedShopItem;
}

UFTItemTileListObject* UFTShopViewModel::GetSelectedPlayerItemObject() const
{
	return SelectedPlayerItem;
}

FText UFTShopViewModel::GetSelectedItemNameText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem ? SelectedItem->GetDisplayName() : FText::FromString(TEXT("Select Item"));
}

FText UFTShopViewModel::GetSelectedItemDescriptionText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem ? SelectedItem->GetDescription() : FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemPriceText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem
		? FText::FromString(FString::Printf(TEXT("Price: %d"), SelectedItem->GetPrice()))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemCountText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem
		? FText::FromString(FString::Printf(TEXT("Count: %d"), SelectedItem->GetCount()))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemStateText() const
{
	if (!GetSelectedItem())
	{
		return FText::GetEmpty();
	}

	return CanBuySelectedItem() || CanSellSelectedItem()
		? FText::FromString(TEXT("Trade Available"))
		: FText::FromString(TEXT("Trade Unavailable"));
}

bool UFTShopViewModel::CanBuySelectedItem() const
{
	return SelectedSource == EFTShopSelectionSource::Shop
		&& ShopSubsystem
		&& SelectedShopItem
		&& ShopSubsystem->CanBuyItem(SelectedShopItem->GetItemID(), PlayerInventory);
}

bool UFTShopViewModel::CanSellSelectedItem() const
{
	return SelectedSource == EFTShopSelectionSource::Player
		&& ShopSubsystem
		&& SelectedPlayerItem
		&& ShopSubsystem->CanSellItemToShop(SelectedPlayerItem->GetItemID(), 1, PlayerInventory);
}

void UFTShopViewModel::RefreshAll()
{
	const FName PreviousShopItemID = SelectedShopItem ? SelectedShopItem->GetItemID() : NAME_None;
	const FName PreviousPlayerItemID = SelectedPlayerItem ? SelectedPlayerItem->GetItemID() : NAME_None;
	const EFTShopSelectionSource PreviousSource = SelectedSource;

	RefreshShopItems();
	RefreshPlayerItems();
	RestoreSelection(PreviousShopItemID, PreviousPlayerItemID, PreviousSource);
	UpdateSelectionChecks();
	NotifyChanged();
}

void UFTShopViewModel::SelectShopItemObject(UObject* ItemObject)
{
	SelectedShopItem = Cast<UFTItemTileListObject>(ItemObject);
	SelectedPlayerItem = nullptr;
	SelectedSource = SelectedShopItem ? EFTShopSelectionSource::Shop : EFTShopSelectionSource::None;
	UpdateSelectionChecks();
	NotifyChanged();
}

void UFTShopViewModel::SelectPlayerItemObject(UObject* ItemObject)
{
	SelectedPlayerItem = Cast<UFTItemTileListObject>(ItemObject);
	SelectedShopItem = nullptr;
	SelectedSource = SelectedPlayerItem ? EFTShopSelectionSource::Player : EFTShopSelectionSource::None;
	UpdateSelectionChecks();
	NotifyChanged();
}

bool UFTShopViewModel::BuySelectedItem()
{
	if (!ShopSubsystem || !SelectedShopItem)
	{
		return false;
	}

	if (!ShopSubsystem->BuyItem(SelectedShopItem->GetItemID(), PlayerInventory))
	{
		return false;
	}

	RefreshAll();
	return true;
}

bool UFTShopViewModel::SellSelectedItem()
{
	if (!ShopSubsystem || !SelectedPlayerItem)
	{
		return false;
	}

	if (!ShopSubsystem->SellItemToShop(SelectedPlayerItem->GetItemID(), 1, PlayerInventory))
	{
		return false;
	}

	ClearSelection();
	RefreshAll();
	return true;
}

void UFTShopViewModel::RefreshShop()
{
	if (ShopSubsystem)
	{
		ShopSubsystem->RefreshShopItems();
	}

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

	if (!ShopSubsystem)
	{
		return;
	}

	TArray<FTShopItemStruct> ShopItems;
	ShopSubsystem->GetShopItems(ShopItems);

	for (const FTShopItemStruct& ShopItem : ShopItems)
	{
		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeShopItem(ShopItem, !ShopSubsystem->IsShopItemUnlocked(ShopItem.GetResolvedItemID()));
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
		const int32 SellPrice = ShopSubsystem ? ShopSubsystem->GetShopSellPrice(InventoryItem.ItemId) : 0;
		ItemObject->InitializeItem(InventoryItem.ItemId, InventoryItem.Quantity, SellPrice);
		PlayerItemObjects.Add(ItemObject);
	}
}

void UFTShopViewModel::RestoreSelection(const FName PreviousShopItemID, const FName PreviousPlayerItemID, const EFTShopSelectionSource PreviousSource)
{
	SelectedShopItem = nullptr;
	SelectedPlayerItem = nullptr;
	SelectedSource = EFTShopSelectionSource::None;

	if (PreviousSource == EFTShopSelectionSource::Shop && !PreviousShopItemID.IsNone())
	{
		for (UObject* ItemObject : ShopItemObjects)
		{
			UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject);
			if (TileObject && TileObject->GetItemID() == PreviousShopItemID)
			{
				SelectedShopItem = TileObject;
				SelectedSource = EFTShopSelectionSource::Shop;
				return;
			}
		}
	}

	if (PreviousSource == EFTShopSelectionSource::Player && !PreviousPlayerItemID.IsNone())
	{
		for (UObject* ItemObject : PlayerItemObjects)
		{
			UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject);
			if (TileObject && TileObject->GetItemID() == PreviousPlayerItemID)
			{
				SelectedPlayerItem = TileObject;
				SelectedSource = EFTShopSelectionSource::Player;
				return;
			}
		}
	}
}

void UFTShopViewModel::UpdateSelectionChecks()
{
	for (UObject* ItemObject : ShopItemObjects)
	{
		if (UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject))
		{
			TileObject->SetChecked(TileObject == SelectedShopItem);
		}
	}

	for (UObject* ItemObject : PlayerItemObjects)
	{
		if (UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject))
		{
			TileObject->SetChecked(TileObject == SelectedPlayerItem);
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

const UFTItemTileListObject* UFTShopViewModel::GetSelectedItem() const
{
	if (SelectedSource == EFTShopSelectionSource::Shop)
	{
		return SelectedShopItem;
	}

	if (SelectedSource == EFTShopSelectionSource::Player)
	{
		return SelectedPlayerItem;
	}

	return nullptr;
}

FName UFTShopViewModel::GetSelectedItemID() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem ? SelectedItem->GetItemID() : NAME_None;
}

void UFTShopViewModel::ClearSelection()
{
	SelectedShopItem = nullptr;
	SelectedPlayerItem = nullptr;
	SelectedSource = EFTShopSelectionSource::None;
	UpdateSelectionChecks();
}

void UFTShopViewModel::NotifyChanged()
{
	OnChanged.Broadcast();
}
