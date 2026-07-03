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
<<<<<<< Updated upstream
	return SelectedShopItemObject;
=======
	return SelectedShopItem;
>>>>>>> Stashed changes
}

UFTItemTileListObject* UFTShopViewModel::GetSelectedPlayerItemObject() const
{
<<<<<<< Updated upstream
	return SelectedPlayerItemObject;
=======
	return SelectedPlayerItem;
>>>>>>> Stashed changes
}

FText UFTShopViewModel::GetSelectedItemNameText() const
{
<<<<<<< Updated upstream
	const UFTItemTileListObject* SelectedItem = GetSelectedItemObject();
=======
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
>>>>>>> Stashed changes
	return SelectedItem ? SelectedItem->GetDisplayName() : FText::FromString(TEXT("Select Item"));
}

FText UFTShopViewModel::GetSelectedItemDescriptionText() const
{
<<<<<<< Updated upstream
	const UFTItemTileListObject* SelectedItem = GetSelectedItemObject();
=======
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
>>>>>>> Stashed changes
	return SelectedItem ? SelectedItem->GetDescription() : FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemPriceText() const
{
<<<<<<< Updated upstream
	const UFTItemTileListObject* SelectedItem = GetSelectedItemObject();
=======
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
>>>>>>> Stashed changes
	return SelectedItem
		? FText::FromString(FString::Printf(TEXT("Price: %d"), SelectedItem->GetPrice()))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemCountText() const
{
<<<<<<< Updated upstream
	const UFTItemTileListObject* SelectedItem = GetSelectedItemObject();
=======
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
>>>>>>> Stashed changes
	return SelectedItem
		? FText::FromString(FString::Printf(TEXT("Count: %d"), SelectedItem->GetCount()))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemStateText() const
{
<<<<<<< Updated upstream
	if (!GetSelectedItemObject())
=======
	if (!GetSelectedItem())
>>>>>>> Stashed changes
	{
		return FText::GetEmpty();
	}

	return CanBuySelectedItem() || CanSellSelectedItem()
		? FText::FromString(TEXT("Trade Available"))
		: FText::FromString(TEXT("Trade Unavailable"));
}

bool UFTShopViewModel::CanBuySelectedItem() const
{
<<<<<<< Updated upstream
	return SelectedSource == EShopSelectionSourceType::Shop
		&& HubShop
		&& SelectedShopItemObject
		&& HubShop->CanBuyItem(SelectedShopItemObject->GetItemID(), PlayerInventory);
=======
	return SelectedSource == EFTShopSelectionSource::Shop
		&& HubShop
		&& HubShop->CanBuyItem(GetSelectedItemID(), PlayerInventory);
>>>>>>> Stashed changes
}

bool UFTShopViewModel::CanSellSelectedItem() const
{
<<<<<<< Updated upstream
	return SelectedSource == EShopSelectionSourceType::Player
		&& HubShop
		&& SelectedPlayerItemObject
		&& HubShop->CanSellItemToShop(SelectedPlayerItemObject->GetItemID(), 1, PlayerInventory);
=======
	return SelectedSource == EFTShopSelectionSource::Player
		&& HubShop
		&& HubShop->CanSellItemToShop(GetSelectedItemID(), 1, PlayerInventory);
>>>>>>> Stashed changes
}

void UFTShopViewModel::RefreshAll()
{
<<<<<<< Updated upstream
	const FName PreviousShopItemID = SelectedShopItemObject ? SelectedShopItemObject->GetItemID() : NAME_None;
	const FName PreviousPlayerItemID = SelectedPlayerItemObject ? SelectedPlayerItemObject->GetItemID() : NAME_None;
	const EShopSelectionSourceType PreviousSource = SelectedSource;

	RefreshShopItems();
	RefreshPlayerItems();
	RestoreSelection(PreviousShopItemID, PreviousPlayerItemID, PreviousSource);
=======
	const FName PreviousShopItemID = SelectedShopItem ? SelectedShopItem->GetItemID() : NAME_None;
	const FName PreviousPlayerItemID = SelectedPlayerItem ? SelectedPlayerItem->GetItemID() : NAME_None;

	RefreshShopItems();
	RefreshPlayerItems();
	RestoreSelection(PreviousShopItemID, PreviousPlayerItemID);
	UpdateSelectionChecks();
>>>>>>> Stashed changes
	NotifyChanged();
}

void UFTShopViewModel::SelectShopItemObject(UObject* ItemObject)
{
<<<<<<< Updated upstream
	SelectedShopItemObject = Cast<UFTItemTileListObject>(ItemObject);
	SelectedPlayerItemObject = nullptr;
	SelectedSource = SelectedShopItemObject ? EShopSelectionSourceType::Shop : EShopSelectionSourceType::None;
	ClearItemChecks();

	if (SelectedShopItemObject)
	{
		SelectedShopItemObject->SetChecked(true);
	}

=======
	SelectedShopItem = Cast<UFTItemTileListObject>(ItemObject);
	SelectedPlayerItem = nullptr;
	SelectedSource = SelectedShopItem ? EFTShopSelectionSource::Shop : EFTShopSelectionSource::None;
	UpdateSelectionChecks();
>>>>>>> Stashed changes
	NotifyChanged();
}

void UFTShopViewModel::SelectPlayerItemObject(UObject* ItemObject)
{
<<<<<<< Updated upstream
	SelectedPlayerItemObject = Cast<UFTItemTileListObject>(ItemObject);
	SelectedShopItemObject = nullptr;
	SelectedSource = SelectedPlayerItemObject ? EShopSelectionSourceType::Player : EShopSelectionSourceType::None;
	ClearItemChecks();

	if (SelectedPlayerItemObject)
	{
		SelectedPlayerItemObject->SetChecked(true);
	}

=======
	SelectedPlayerItem = Cast<UFTItemTileListObject>(ItemObject);
	SelectedShopItem = nullptr;
	SelectedSource = SelectedPlayerItem ? EFTShopSelectionSource::Player : EFTShopSelectionSource::None;
	UpdateSelectionChecks();
>>>>>>> Stashed changes
	NotifyChanged();
}

bool UFTShopViewModel::BuySelectedItem()
{
<<<<<<< Updated upstream
	if (!HubShop || !SelectedShopItemObject)
=======
	if (!HubShop || !SelectedShopItem)
>>>>>>> Stashed changes
	{
		return false;
	}

<<<<<<< Updated upstream
	if (!HubShop->BuyItem(SelectedShopItemObject->GetItemID(), PlayerInventory))
=======
	if (!HubShop->BuyItem(SelectedShopItem->GetItemID(), PlayerInventory))
>>>>>>> Stashed changes
	{
		return false;
	}

	RefreshAll();
	return true;
}

bool UFTShopViewModel::SellSelectedItem()
{
<<<<<<< Updated upstream
	if (!HubShop || !SelectedPlayerItemObject)
=======
	if (!HubShop || !SelectedPlayerItem)
>>>>>>> Stashed changes
	{
		return false;
	}

<<<<<<< Updated upstream
	if (!HubShop->SellItemToShop(SelectedPlayerItemObject->GetItemID(), 1, PlayerInventory))
=======
	if (!HubShop->SellItemToShop(SelectedPlayerItem->GetItemID(), 1, PlayerInventory))
>>>>>>> Stashed changes
	{
		return false;
	}

	ClearSelection();
	RefreshAll();
	return true;
}

<<<<<<< Updated upstream
void UFTShopViewModel::RefreshShopStock()
{
	if (!HubShop)
	{
		return;
	}

	HubShop->RefreshShopItems();
=======
void UFTShopViewModel::RefreshShop()
{
	if (HubShop)
	{
		HubShop->RefreshShopItems();
	}

>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
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
=======
void UFTShopViewModel::RestoreSelection(const FName PreviousShopItemID, const FName PreviousPlayerItemID)
{
	SelectedShopItem = nullptr;
	SelectedPlayerItem = nullptr;

	if (SelectedSource == EFTShopSelectionSource::Shop && !PreviousShopItemID.IsNone())
>>>>>>> Stashed changes
	{
		for (UObject* ItemObject : ShopItemObjects)
		{
			UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject);
			if (TileObject && TileObject->GetItemID() == PreviousShopItemID)
			{
<<<<<<< Updated upstream
				SelectedShopItemObject = TileObject;
				SelectedSource = EShopSelectionSourceType::Shop;
				TileObject->SetChecked(true);
=======
				SelectedShopItem = TileObject;
>>>>>>> Stashed changes
				return;
			}
		}
	}

<<<<<<< Updated upstream
	if (PreviousSource == EShopSelectionSourceType::Player && !PreviousPlayerItemID.IsNone())
=======
	if (SelectedSource == EFTShopSelectionSource::Player && !PreviousPlayerItemID.IsNone())
>>>>>>> Stashed changes
	{
		for (UObject* ItemObject : PlayerItemObjects)
		{
			UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject);
			if (TileObject && TileObject->GetItemID() == PreviousPlayerItemID)
			{
<<<<<<< Updated upstream
				SelectedPlayerItemObject = TileObject;
				SelectedSource = EShopSelectionSourceType::Player;
				TileObject->SetChecked(true);
=======
				SelectedPlayerItem = TileObject;
>>>>>>> Stashed changes
				return;
			}
		}
	}
<<<<<<< Updated upstream
}

void UFTShopViewModel::ClearSelection()
{
	SelectedShopItemObject = nullptr;
	SelectedPlayerItemObject = nullptr;
	SelectedSource = EShopSelectionSourceType::None;
	ClearItemChecks();
}

void UFTShopViewModel::ClearItemChecks()
=======

	SelectedSource = EFTShopSelectionSource::None;
}

void UFTShopViewModel::UpdateSelectionChecks()
>>>>>>> Stashed changes
{
	for (UObject* ItemObject : ShopItemObjects)
	{
		if (UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject))
		{
<<<<<<< Updated upstream
			TileObject->SetChecked(false);
=======
			TileObject->SetChecked(TileObject == SelectedShopItem);
>>>>>>> Stashed changes
		}
	}

	for (UObject* ItemObject : PlayerItemObjects)
	{
		if (UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject))
		{
<<<<<<< Updated upstream
			TileObject->SetChecked(false);
=======
			TileObject->SetChecked(TileObject == SelectedPlayerItem);
>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
UFTItemTileListObject* UFTShopViewModel::GetSelectedItemObject() const
{
	if (SelectedSource == EShopSelectionSourceType::Shop)
	{
		return SelectedShopItemObject;
	}

	if (SelectedSource == EShopSelectionSourceType::Player)
	{
		return SelectedPlayerItemObject;
=======
const UFTItemTileListObject* UFTShopViewModel::GetSelectedItem() const
{
	if (SelectedSource == EFTShopSelectionSource::Shop)
	{
		return SelectedShopItem;
	}

	if (SelectedSource == EFTShopSelectionSource::Player)
	{
		return SelectedPlayerItem;
>>>>>>> Stashed changes
	}

	return nullptr;
}

<<<<<<< Updated upstream
=======
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
}

>>>>>>> Stashed changes
void UFTShopViewModel::NotifyChanged()
{
	OnChanged.Broadcast();
}
