#include "FTStorageSubsystem.h"

#include "ProjectFT/Components/FTInventoryComponent.h"

void UFTStorageSubsystem::InitializeStorage(UFTInventoryComponent* StorageInventory, const TArray<FTStorageItemStruct>& InitialItems) const
{
	if (!StorageInventory)
	{
		return;
	}

	for (const FTStorageItemStruct& InitialItem : InitialItems)
	{
		if (!InitialItem.ItemID.IsNone() && InitialItem.Count > 0)
		{
			AddStorageItem(StorageInventory, InitialItem.ItemID, InitialItem.Count);
		}
	}
}

bool UFTStorageSubsystem::AddStorageItem(UFTInventoryComponent* StorageInventory, const FName ItemID, const int32 Count) const
{
	return StorageInventory
		? StorageInventory->AddItem(ItemID, Count)
		: false;
}

bool UFTStorageSubsystem::RemoveStorageItem(UFTInventoryComponent* StorageInventory, const FName ItemID, const int32 Count) const
{
	return StorageInventory
		? StorageInventory->RemoveItem(ItemID, Count)
		: false;
}

int32 UFTStorageSubsystem::GetStorageItemCount(const UFTInventoryComponent* StorageInventory, const FName ItemID) const
{
	return StorageInventory
		? StorageInventory->GetItemQuantity(ItemID)
		: 0;
}

void UFTStorageSubsystem::GetStorageItems(const UFTInventoryComponent* StorageInventory, TArray<FTStorageItemStruct>& OutItems) const
{
	OutItems.Reset();

	if (!StorageInventory)
	{
		return;
	}

	for (const FFTInventoryItem& Item : StorageInventory->GetItems())
	{
		OutItems.Add({ Item.ItemId, Item.Quantity });
	}
}

bool UFTStorageSubsystem::StoreItemFromInventory(UFTInventoryComponent* StorageInventory, UFTInventoryComponent* SourceInventory, const FName ItemID, const int32 Count) const
{
	if (!SourceInventory || !StorageInventory || ItemID.IsNone() || Count <= 0)
	{
		return false;
	}

	if (SourceInventory->GetItemQuantity(ItemID) < Count)
	{
		UE_LOG(LogTemp, Warning, TEXT("Store Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!AddStorageItem(StorageInventory, ItemID, Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Store Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!SourceInventory->RemoveItem(ItemID, Count))
	{
		RemoveStorageItem(StorageInventory, ItemID, Count);
		UE_LOG(LogTemp, Warning, TEXT("Store Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Store Success: %s x%d"), *ItemID.ToString(), Count);
	return true;
}

bool UFTStorageSubsystem::TakeItemToInventory(UFTInventoryComponent* StorageInventory, UFTInventoryComponent* TargetInventory, const FName ItemID, const int32 Count) const
{
	if (!TargetInventory || !StorageInventory || ItemID.IsNone() || Count <= 0)
	{
		return false;
	}

	if (GetStorageItemCount(StorageInventory, ItemID) < Count)
	{
		UE_LOG(LogTemp, Warning, TEXT("Take Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!TargetInventory->AddItem(ItemID, Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Take Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!RemoveStorageItem(StorageInventory, ItemID, Count))
	{
		TargetInventory->RemoveItem(ItemID, Count);
		UE_LOG(LogTemp, Warning, TEXT("Take Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Take Success: %s x%d"), *ItemID.ToString(), Count);
	return true;
}

int32 UFTStorageSubsystem::GetCombinedItemCount(const UFTInventoryComponent* PlayerInventory, const UFTInventoryComponent* StorageInventory, const FName ItemID) const
{
	int32 Count = 0;

	if (PlayerInventory)
	{
		Count += PlayerInventory->GetItemQuantity(ItemID);
	}

	if (StorageInventory)
	{
		Count += GetStorageItemCount(StorageInventory, ItemID);
	}

	return Count;
}

bool UFTStorageSubsystem::ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, UFTInventoryComponent* StorageInventory, const FName ItemID, const int32 Count) const
{
	if (ItemID.IsNone() || Count <= 0 || GetCombinedItemCount(PlayerInventory, StorageInventory, ItemID) < Count)
	{
		return false;
	}

	int32 RemainingCount = Count;

	if (PlayerInventory)
	{
		const int32 PlayerCount = PlayerInventory->GetItemQuantity(ItemID);
		const int32 RemoveFromPlayer = FMath::Min(PlayerCount, RemainingCount);

		if (RemoveFromPlayer > 0 && PlayerInventory->RemoveItem(ItemID, RemoveFromPlayer))
		{
			RemainingCount -= RemoveFromPlayer;
		}
	}

	if (RemainingCount > 0 && StorageInventory)
	{
		const int32 StorageCount = GetStorageItemCount(StorageInventory, ItemID);
		const int32 RemoveFromStorage = FMath::Min(StorageCount, RemainingCount);

		if (RemoveFromStorage > 0 && RemoveStorageItem(StorageInventory, ItemID, RemoveFromStorage))
		{
			RemainingCount -= RemoveFromStorage;
		}
	}

	return RemainingCount <= 0;
}

void UFTStorageSubsystem::PrintStorageItems(const UFTInventoryComponent* StorageInventory) const
{
	TArray<FTStorageItemStruct> StorageItems;
	GetStorageItems(StorageInventory, StorageItems);

	if (StorageItems.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Storage Empty"));
		return;
	}

	for (const FTStorageItemStruct& StorageItem : StorageItems)
	{
		UE_LOG(LogTemp, Warning, TEXT("Storage: %s x%d"),
			*StorageItem.ItemID.ToString(),
			StorageItem.Count
		);
	}
}
