#include "FTHubStorageViewModel.h"

#include "Engine/AssetManager.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/UI/HubUI/FTItemTileListObject.h"

void UFTHubStorageViewModel::Initialize(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventoryDelegates();

	HubStorage = InHubStorage;
	PlayerInventory = InPlayerInventory;
	ClearSelection();

	BindInventoryDelegates();
	RefreshAll();
}

const TArray<TObjectPtr<UObject>>& UFTHubStorageViewModel::GetPlayerItemObjects() const
{
	return PlayerItemObjects;
}

const TArray<TObjectPtr<UObject>>& UFTHubStorageViewModel::GetStorageItemObjects() const
{
	return StorageItemObjects;
}

FText UFTHubStorageViewModel::GetSelectedItemText() const
{
	const int32 SelectedCount = GetSelectedEntryCount();
	return SelectedCount > 0
		? FText::FromString(FString::Printf(TEXT("Selected Items: %d"), SelectedCount))
		: FText::FromString(TEXT("Select Item"));
}

FText UFTHubStorageViewModel::GetPlayerWeightText() const
{
	const float CurrentWeight = PlayerInventory ? PlayerInventory->GetCurrentWeight() : 0.0f;
	const float MaxWeight = PlayerInventory ? PlayerInventory->GetMaxWeight() : 0.0f;
	return FText::FromString(FString::Printf(TEXT("%.1f / %.1f kg"), CurrentWeight, MaxWeight));
}

int32 UFTHubStorageViewModel::GetSelectedEntryCount() const
{
	return SelectedItems.Num();
}

int32 UFTHubStorageViewModel::GetPlayerSelectedEntryCount() const
{
	return SelectedSource == EFTHubStorageTransferSource::Player ? SelectedItems.Num() : 0;
}

int32 UFTHubStorageViewModel::GetStorageSelectedEntryCount() const
{
	return SelectedSource == EFTHubStorageTransferSource::Storage ? SelectedItems.Num() : 0;
}

bool UFTHubStorageViewModel::CanStoreSelected() const
{
	return GetPlayerSelectedEntryCount() > 0;
}

bool UFTHubStorageViewModel::CanTakeSelected() const
{
	return GetStorageSelectedEntryCount() > 0;
}

bool UFTHubStorageViewModel::CanStoreAll() const
{
	return PlayerInventory && PlayerInventory->GetItems().Num() > 0;
}

bool UFTHubStorageViewModel::CanTakeAll() const
{
	TArray<FTStorageItemStruct> StorageItems;
	GetCurrentStorageItems(StorageItems);
	return !StorageItems.IsEmpty();
}

void UFTHubStorageViewModel::RefreshAll()
{
	RefreshPlayerItems();
	RefreshStorageItems();
	OnChanged.Broadcast();
}

void UFTHubStorageViewModel::SetSelectedItems(const EFTHubStorageTransferSource SourceType, const TArray<UObject*>& Items)
{
	SelectedItems.Reset();
	SelectedSource = SourceType;

	for (UObject* ItemObject : Items)
	{
		FTStorageItemStruct StorageItem;
		if (TryReadItemObject(ItemObject, StorageItem))
		{
			SelectedItems.Add(StorageItem);
		}
	}

	if (SelectedItems.IsEmpty())
	{
		SelectedSource = EFTHubStorageTransferSource::None;
	}

	OnChanged.Broadcast();
}

void UFTHubStorageViewModel::ClearSelection()
{
	SelectedItems.Reset();
	SelectedSource = EFTHubStorageTransferSource::None;
}

void UFTHubStorageViewModel::SetPlayerFilter(const EFTItemCategoryType FilterCategory)
{
	PlayerFilterCategory = FilterCategory;
	ClearSelection();
	RefreshAll();
}

void UFTHubStorageViewModel::SetStorageFilter(const EFTItemCategoryType FilterCategory)
{
	StorageFilterCategory = FilterCategory;
	ClearSelection();
	RefreshAll();
}

bool UFTHubStorageViewModel::StoreSelectedItems()
{
	return TransferSelectedItems(EFTHubStorageTransferSource::Player);
}

bool UFTHubStorageViewModel::TakeSelectedItems()
{
	return TransferSelectedItems(EFTHubStorageTransferSource::Storage);
}

bool UFTHubStorageViewModel::StoreAllItems()
{
	return TransferAllItems(EFTHubStorageTransferSource::Player);
}

bool UFTHubStorageViewModel::TakeAllItems()
{
	return TransferAllItems(EFTHubStorageTransferSource::Storage);
}

void UFTHubStorageViewModel::HandleInventoryChanged()
{
	RefreshAll();
}

void UFTHubStorageViewModel::RefreshPlayerItems()
{
	PlayerItemObjects.Reset();

	if (!PlayerInventory)
	{
		return;
	}

	for (const FFTInventoryItem& InventoryItem : PlayerInventory->GetItems())
	{
		if (!ShouldShowItem(InventoryItem.ItemId, PlayerFilterCategory))
		{
			continue;
		}

		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeItem(InventoryItem.ItemId, InventoryItem.Quantity);
		PlayerItemObjects.Add(ItemObject);
	}
}

void UFTHubStorageViewModel::RefreshStorageItems()
{
	StorageItemObjects.Reset();

	if (!HubStorage)
	{
		return;
	}

	TArray<FTStorageItemStruct> StorageItems;
	GetCurrentStorageItems(StorageItems);
	for (const FTStorageItemStruct& StorageItem : StorageItems)
	{
		if (!ShouldShowItem(StorageItem.ItemID, StorageFilterCategory))
		{
			continue;
		}

		UFTItemTileListObject* ItemObject = NewObject<UFTItemTileListObject>(this);
		ItemObject->InitializeItem(StorageItem.ItemID, StorageItem.Count);
		StorageItemObjects.Add(ItemObject);
	}
}

void UFTHubStorageViewModel::BindInventoryDelegates()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTHubStorageViewModel::HandleInventoryChanged);
		PlayerInventory->OnInventoryChanged.AddDynamic(this, &UFTHubStorageViewModel::HandleInventoryChanged);
	}

	if (UFTInventoryComponent* StorageInventory = GetStorageInventory())
	{
		StorageInventory->OnInventoryChanged.RemoveDynamic(this, &UFTHubStorageViewModel::HandleInventoryChanged);
		StorageInventory->OnInventoryChanged.AddDynamic(this, &UFTHubStorageViewModel::HandleInventoryChanged);
	}
}

void UFTHubStorageViewModel::UnbindInventoryDelegates()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTHubStorageViewModel::HandleInventoryChanged);
	}

	if (UFTInventoryComponent* StorageInventory = GetStorageInventory())
	{
		StorageInventory->OnInventoryChanged.RemoveDynamic(this, &UFTHubStorageViewModel::HandleInventoryChanged);
	}
}

UFTStorageSubsystem* UFTHubStorageViewModel::GetStorageSubsystem() const
{
	return HubStorage && HubStorage->GetGameInstance()
		? HubStorage->GetGameInstance()->GetSubsystem<UFTStorageSubsystem>()
		: nullptr;
}

UFTInventoryComponent* UFTHubStorageViewModel::GetStorageInventory() const
{
	return HubStorage ? HubStorage->GetStorageInventory() : nullptr;
}

void UFTHubStorageViewModel::GetCurrentStorageItems(TArray<FTStorageItemStruct>& OutItems) const
{
	OutItems.Reset();

	if (const UFTStorageSubsystem* StorageSubsystem = GetStorageSubsystem())
	{
		StorageSubsystem->GetStorageItems(GetStorageInventory(), OutItems);
	}
}

bool UFTHubStorageViewModel::ShouldShowItem(const FName ItemID, const EFTItemCategoryType FilterCategory) const
{
	return FilterCategory == EFTItemCategoryType::None || GetItemCategory(ItemID) == FilterCategory;
}

EFTItemCategoryType UFTHubStorageViewModel::GetItemCategory(const FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return EFTItemCategoryType::None;
	}

	const UFTItemDataAsset* ItemDataAsset = PlayerInventory
		? PlayerInventory->FindItemData(ItemID)
		: nullptr;

	if (!ItemDataAsset)
	{
		UAssetManager& AssetManager = UAssetManager::Get();
		const FPrimaryAssetId AssetID(FName("FTItemItem"), ItemID);
		UObject* AssetObject = AssetManager.GetPrimaryAssetObject(AssetID);
		if (!AssetObject)
		{
			const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetID);
			if (AssetPath.IsValid())
			{
				AssetObject = AssetPath.TryLoad();
			}
		}

		ItemDataAsset = Cast<UFTItemDataAsset>(AssetObject);
	}

	return ItemDataAsset ? ItemDataAsset->ItemData.CategoryType : EFTItemCategoryType::None;
}

bool UFTHubStorageViewModel::TryReadItemObject(UObject* ItemObject, FTStorageItemStruct& OutItem) const
{
	if (const UFTItemTileListObject* TileObject = Cast<UFTItemTileListObject>(ItemObject))
	{
		OutItem.ItemID = TileObject->GetItemID();
		OutItem.Count = TileObject->GetCount();
		return !OutItem.ItemID.IsNone() && OutItem.Count > 0;
	}

	OutItem = {};
	return false;
}

bool UFTHubStorageViewModel::TransferSelectedItems(const EFTHubStorageTransferSource SourceType)
{
	UFTStorageSubsystem* StorageSubsystem = GetStorageSubsystem();
	UFTInventoryComponent* StorageInventory = GetStorageInventory();
	if (!StorageSubsystem || !StorageInventory || !PlayerInventory || SourceType == EFTHubStorageTransferSource::None || SelectedSource != SourceType)
	{
		return false;
	}

	bool bMovedAnyItem = false;
	for (const FTStorageItemStruct& SelectedItem : SelectedItems)
	{
		if (SelectedItem.ItemID.IsNone() || SelectedItem.Count <= 0)
		{
			continue;
		}

		const bool bMoved = SourceType == EFTHubStorageTransferSource::Player
			? StorageSubsystem->StoreItemFromInventory(StorageInventory, PlayerInventory, SelectedItem.ItemID, SelectedItem.Count)
			: StorageSubsystem->TakeItemToInventory(StorageInventory, PlayerInventory, SelectedItem.ItemID, SelectedItem.Count);

		bMovedAnyItem = bMovedAnyItem || bMoved;
	}

	if (bMovedAnyItem)
	{
		ClearSelection();
		RefreshAll();
	}

	return bMovedAnyItem;
}

bool UFTHubStorageViewModel::TransferAllItems(const EFTHubStorageTransferSource SourceType)
{
	UFTStorageSubsystem* StorageSubsystem = GetStorageSubsystem();
	UFTInventoryComponent* StorageInventory = GetStorageInventory();
	if (!StorageSubsystem || !StorageInventory || !PlayerInventory || SourceType == EFTHubStorageTransferSource::None)
	{
		return false;
	}

	TArray<FTStorageItemStruct> ItemsToMove;
	if (SourceType == EFTHubStorageTransferSource::Player)
	{
		for (const FFTInventoryItem& InventoryItem : PlayerInventory->GetItems())
		{
			ItemsToMove.Add({ InventoryItem.ItemId, InventoryItem.Quantity });
		}
	}
	else
	{
		StorageSubsystem->GetStorageItems(StorageInventory, ItemsToMove);
	}

	bool bMovedAnyItem = false;
	for (const FTStorageItemStruct& Item : ItemsToMove)
	{
		if (Item.ItemID.IsNone() || Item.Count <= 0)
		{
			continue;
		}

		const bool bMoved = SourceType == EFTHubStorageTransferSource::Player
			? StorageSubsystem->StoreItemFromInventory(StorageInventory, PlayerInventory, Item.ItemID, Item.Count)
			: StorageSubsystem->TakeItemToInventory(StorageInventory, PlayerInventory, Item.ItemID, Item.Count);

		bMovedAnyItem = bMovedAnyItem || bMoved;
	}

	if (bMovedAnyItem)
	{
		ClearSelection();
		RefreshAll();
	}

	return bMovedAnyItem;
}
