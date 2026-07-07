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

const TArray<TObjectPtr<UObject>>& UFTShopViewModel::GetCurrentItemObjects() const
{
	return CurrentMode == EFTShopPanelMode::Buy ? ShopItemObjects : PlayerItemObjects;
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

FText UFTShopViewModel::GetSelectedItemNameText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem ? SelectedItem->GetDisplayName() : FText::FromString(TEXT("Select Item"));
}

FText UFTShopViewModel::GetSelectedItemTagText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem ? SelectedItem->GetCategoryText() : FText::GetEmpty();
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
		? FText::FromString(FString::Printf(TEXT("개당 가격: %d"), GetUnitPrice()))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemCountText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem
		? FText::FromString(FString::Printf(TEXT("수량: %d"), SelectedItem->GetCount()))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetSelectedItemOwnedCountText() const
{
	const FName ItemID = GetSelectedItemID();
	if (ItemID.IsNone() || !PlayerInventory)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Printf(TEXT("보유: %d"), PlayerInventory->GetItemQuantity(ItemID)));
}

FText UFTShopViewModel::GetTradeQuantityText() const
{
	return FText::AsNumber(TradeQuantity);
}

FText UFTShopViewModel::GetTradeTotalPriceText() const
{
	const UFTItemTileListObject* SelectedItem = GetSelectedItem();
	return SelectedItem
		? FText::FromString(FString::Printf(TEXT("총 가격: %d"), GetUnitPrice() * TradeQuantity))
		: FText::GetEmpty();
}

FText UFTShopViewModel::GetTradeActionText() const
{
	return CurrentMode == EFTShopPanelMode::Buy
		? FText::FromString(TEXT("구매하기"))
		: FText::FromString(TEXT("판매하기"));
}

FText UFTShopViewModel::GetSelectedItemStateText() const
{
	if (!GetSelectedItem())
	{
		return FText::GetEmpty();
	}

	return CanExecuteTradeAction()
		? FText::FromString(TEXT("거래 가능"))
		: FText::FromString(TEXT("거래 불가"));
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
{
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

bool UFTShopViewModel::BuySelectedItem()
{
	if (!ShopSubsystem || !SelectedShopItem)
	{
		return false;
	}

	for (int32 Index = 0; Index < TradeQuantity; ++Index)
	{
		if (!ShopSubsystem->BuyItem(SelectedShopItem->GetItemID(), PlayerInventory))
		{
			RefreshAll();
			return false;
		}
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

	if (!ShopSubsystem->SellItemToShop(SelectedPlayerItem->GetItemID(), TradeQuantity, PlayerInventory))
	{
		return false;
	}

	ClearSelection();
	RefreshAll();
	return true;
}

bool UFTShopViewModel::ExecuteTradeAction()
{
	return CurrentMode == EFTShopPanelMode::Buy ? BuySelectedItem() : SellSelectedItem();
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

	return FMath::Clamp(ShopSubsystem->GetCurrencyAmount(PlayerInventory) / UnitPrice, 1, 99);
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
