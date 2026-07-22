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

TArray<UObject*> UFTShopViewModel::GetShopItemObjects() const
{
	TArray<UObject*> Result;
	Result.Reserve(ShopItemObjects.Num());
	for (UObject* Item : ShopItemObjects)
	{
		Result.Add(Item);
	}
	return Result;
}

TArray<UObject*> UFTShopViewModel::GetPlayerItemObjects() const
{
	TArray<UObject*> Result;
	Result.Reserve(PlayerItemObjects.Num());
	for (UObject* Item : PlayerItemObjects)
	{
		Result.Add(Item);
	}
	return Result;
}

TArray<UObject*> UFTShopViewModel::GetCurrentItemObjects() const
{
	return CurrentMode == EFTShopPanelMode::Buy ? GetShopItemObjects() : GetPlayerItemObjects();
}

UFTItemTileListObject* UFTShopViewModel::GetSelectedShopItemObject() const
{
	return SelectedShopItem;
}

UFTItemTileListObject* UFTShopViewModel::GetSelectedPlayerItemObject() const
{
	return SelectedPlayerItem;
}

UFTItemTileListObject* UFTShopViewModel::GetSelectedCurrentItemObject() const
{
	return CurrentMode == EFTShopPanelMode::Buy ? SelectedShopItem : SelectedPlayerItem;
}

bool UFTShopViewModel::HasSelectedItem() const
{
	return GetSelectedItem() != nullptr;
}

int32 UFTShopViewModel::GetSelectedItemUnitPrice() const
{
	return HasSelectedItem() ? GetUnitPrice() : 0;
}

int32 UFTShopViewModel::GetSelectedItemOwnedCount() const
{
	const FName ItemID = GetSelectedItemID();
	return !ItemID.IsNone() && PlayerInventory ? PlayerInventory->GetItemQuantity(ItemID) : 0;
}

int32 UFTShopViewModel::GetTradeQuantity() const
{
	return TradeQuantity;
}

int32 UFTShopViewModel::GetTradeTotalPrice() const
{
	return HasSelectedItem() ? GetUnitPrice() * TradeQuantity : 0;
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

TSoftObjectPtr<UTexture2D> UFTShopViewModel::GetSelectedItemIcon() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem ? SelectedItem->GetItemIcon() : TSoftObjectPtr<UTexture2D>();
}

bool UFTShopViewModel::IsBuyMode() const
{
	return CurrentMode == EFTShopPanelMode::Buy;
}

bool UFTShopViewModel::IsSellMode() const
{
	return CurrentMode == EFTShopPanelMode::Sell;
}

bool UFTShopViewModel::CanBuySelectedItem() const
{
	return SelectedSource == EFTShopSelectionSource::Shop
		&& ShopSubsystem
		&& SelectedShopItem
		&& ShopSubsystem->CanBuyItem(SelectedShopItem->GetItemID(), PlayerInventory)
		&& ShopSubsystem->GetCurrencyAmount(PlayerInventory) >= GetUnitPrice() * TradeQuantity;
}

bool UFTShopViewModel::CanSellSelectedItem() const
{
	return SelectedSource == EFTShopSelectionSource::Player
		&& ShopSubsystem
		&& SelectedPlayerItem
		&& ShopSubsystem->CanSellItemToShop(SelectedPlayerItem->GetItemID(), TradeQuantity, PlayerInventory);
}

bool UFTShopViewModel::CanExecuteTradeAction() const
{
	return CurrentMode == EFTShopPanelMode::Buy ? CanBuySelectedItem() : CanSellSelectedItem();
}

bool UFTShopViewModel::CanSetTradeQuantityToHalf() const
{
	if (!GetSelectedItem())
	{
		return false;
	}

	const int32 MaxTradeQuantity = GetMaxTradeQuantity();
	const int32 HalfTradeQuantity = FMath::Clamp(MaxTradeQuantity / 2, 1, MaxTradeQuantity);
	return MaxTradeQuantity > 1 && TradeQuantity != HalfTradeQuantity;
}

bool UFTShopViewModel::CanSetTradeQuantityToMax() const
{
	return GetSelectedItem() && TradeQuantity < GetMaxTradeQuantity();
}

void UFTShopViewModel::RefreshAll()
{
	const FName PreviousShopItemID = SelectedShopItem ? SelectedShopItem->GetItemID() : NAME_None;
	const FName PreviousPlayerItemID = SelectedPlayerItem ? SelectedPlayerItem->GetItemID() : NAME_None;
	const EFTShopSelectionSource PreviousSource = SelectedSource;

	RefreshShopItems();
	RefreshPlayerItems();
	RestoreSelection(PreviousShopItemID, PreviousPlayerItemID, PreviousSource);
	TradeQuantity = FMath::Clamp(TradeQuantity, 1, GetMaxTradeQuantity());
	UpdateSelectionChecks();
	NotifyChanged();
}

void UFTShopViewModel::SetBuyMode()
0{
	if (CurrentMode == EFTShopPanelMode::Buy)
	{
		return;
	}

	CurrentMode = EFTShopPanelMode::Buy;
	SelectedPlayerItem = nullptr;
	SelectedSource = SelectedShopItem ? EFTShopSelectionSource::Shop : EFTShopSelectionSource::None;
	ResetTradeQuantity();
	UpdateSelectionChecks();
	NotifyChanged();
}

void UFTShopViewModel::SetSellMode()
{
	if (CurrentMode == EFTShopPanelMode::Sell)
	{
		return;
	}

	CurrentMode = EFTShopPanelMode::Sell;
	SelectedShopItem = nullptr;
	SelectedSource = SelectedPlayerItem ? EFTShopSelectionSource::Player : EFTShopSelectionSource::None;
	ResetTradeQuantity();
	UpdateSelectionChecks();
	NotifyChanged();
}

void UFTShopViewModel::SelectShopItemObject(UObject* ItemObject)
{
	SelectedShopItem = Cast<UFTItemTileListObject>(ItemObject);
	SelectedPlayerItem = nullptr;
	SelectedSource = SelectedShopItem ? EFTShopSelectionSource::Shop : EFTShopSelectionSource::None;
	CurrentMode = EFTShopPanelMode::Buy;
	ResetTradeQuantity();
	UpdateSelectionChecks();
	NotifyChanged();
}

void UFTShopViewModel::SelectPlayerItemObject(UObject* ItemObject)
{
	SelectedPlayerItem = Cast<UFTItemTileListObject>(ItemObject);
	SelectedShopItem = nullptr;
	SelectedSource = SelectedPlayerItem ? EFTShopSelectionSource::Player : EFTShopSelectionSource::None;
	CurrentMode = EFTShopPanelMode::Sell;
	ResetTradeQuantity();
	UpdateSelectionChecks();
	NotifyChanged();
}

void UFTShopViewModel::SelectCurrentItemObject(UObject* ItemObject)
{
	if (CurrentMode == EFTShopPanelMode::Buy)
	{
		SelectShopItemObject(ItemObject);
	}
	else
	{
		SelectPlayerItemObject(ItemObject);
	}
}

void UFTShopViewModel::IncreaseTradeQuantity()
{
	TradeQuantity = FMath::Clamp(TradeQuantity + 1, 1, GetMaxTradeQuantity());
	NotifyChanged();
}

void UFTShopViewModel::DecreaseTradeQuantity()
{
	TradeQuantity = FMath::Clamp(TradeQuantity - 1, 1, GetMaxTradeQuantity());
	NotifyChanged();
}

void UFTShopViewModel::SetTradeQuantityToHalf()
{
	const int32 MaxTradeQuantity = GetMaxTradeQuantity();
	TradeQuantity = FMath::Clamp(MaxTradeQuantity / 2, 1, MaxTradeQuantity);
	NotifyChanged();
}

void UFTShopViewModel::SetTradeQuantityToMax()
{
	TradeQuantity = GetMaxTradeQuantity();
	NotifyChanged();
}

bool UFTShopViewModel::BuySelectedItem()
{
	if (!ShopSubsystem || !SelectedShopItem)
	{
		return false;
	}

	bTransactionInProgress = true;
	const bool bPurchased = ShopSubsystem->BuyItemCount(SelectedShopItem->GetItemID(), TradeQuantity, PlayerInventory);
	bTransactionInProgress = false;
	RefreshAll();
	return bPurchased;
}

bool UFTShopViewModel::SellSelectedItem()
{
	if (!ShopSubsystem || !SelectedPlayerItem)
	{
		return false;
	}

	bTransactionInProgress = true;
	const bool bSold = ShopSubsystem->SellItemToShop(SelectedPlayerItem->GetItemID(), TradeQuantity, PlayerInventory);
	bTransactionInProgress = false;
	if (bSold)
	{
		ClearSelection();
	}

	RefreshAll();
	return bSold;
}

bool UFTShopViewModel::ExecuteTradeAction()
{
	return CurrentMode == EFTShopPanelMode::Buy ? BuySelectedItem() : SellSelectedItem();
}

void UFTShopViewModel::RefreshShop()
{
	ClearSelection();
	RefreshAll();
}

void UFTShopViewModel::HandleInventoryChanged()
{
	if (!bTransactionInProgress)
	{
		RefreshAll();
	}
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
		if (!ShopSubsystem || !ShopSubsystem->IsItemSellableToShop(InventoryItem.ItemId))
		{
			continue;
		}

		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		const int32 SellPrice = ShopSubsystem->GetShopSellPrice(InventoryItem.ItemId);
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
				CurrentMode = EFTShopPanelMode::Buy;
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
				CurrentMode = EFTShopPanelMode::Sell;
				return;
			}
		}
	}

	if (CurrentMode == EFTShopPanelMode::Buy && !ShopItemObjects.IsEmpty())
	{
		SelectedShopItem = Cast<UFTItemTileListObject>(ShopItemObjects[0]);
		SelectedSource = SelectedShopItem ? EFTShopSelectionSource::Shop : EFTShopSelectionSource::None;
	}
	else if (CurrentMode == EFTShopPanelMode::Sell && !PlayerItemObjects.IsEmpty())
	{
		SelectedPlayerItem = Cast<UFTItemTileListObject>(PlayerItemObjects[0]);
		SelectedSource = SelectedPlayerItem ? EFTShopSelectionSource::Player : EFTShopSelectionSource::None;
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

int32 UFTShopViewModel::GetMaxTradeQuantity() const
{
	if (!GetSelectedItem())
	{
		return 1;
	}

	if (CurrentMode == EFTShopPanelMode::Sell)
	{
		return FMath::Max(1, SelectedPlayerItem ? SelectedPlayerItem->GetCount() : 1);
	}

	const int32 UnitPrice = GetUnitPrice();
	if (!ShopSubsystem || UnitPrice <= 0)
	{
		return 99;
	}

	return FMath::Max(1, ShopSubsystem->GetCurrencyAmount(PlayerInventory) / UnitPrice);
}

int32 UFTShopViewModel::GetUnitPrice() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem ? FMath::Max(0, SelectedItem->GetPrice()) : 0;
}

void UFTShopViewModel::ResetTradeQuantity()
{
	TradeQuantity = FMath::Clamp(1, 1, GetMaxTradeQuantity());
}

void UFTShopViewModel::ClearSelection()
{
	SelectedShopItem = nullptr;
	SelectedPlayerItem = nullptr;
	SelectedSource = EFTShopSelectionSource::None;
	ResetTradeQuantity();
	UpdateSelectionChecks();
}

void UFTShopViewModel::NotifyChanged()
{
	OnChanged.Broadcast();
}
