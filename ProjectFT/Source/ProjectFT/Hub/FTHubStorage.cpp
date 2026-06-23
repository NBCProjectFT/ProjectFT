#include "FTHubStorage.h"

AFTHubStorage::AFTHubStorage()
{
	PrimaryActorTick.bCanEverTick = false;

	TestStorageItems.Add({ TEXT("Water"), 3 });
	TestStorageItems.Add({ TEXT("Sugar"), 2 });
}

void AFTHubStorage::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("Before Storage Test"));
	PrintStorageItems();
}

void AFTHubStorage::AddStorageItem(FName ItemID, int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	for (FTStorageItemStruct& StorageItem : TestStorageItems)
	{
		if (StorageItem.ItemID == ItemID)
		{
			StorageItem.Count += Count;
			return;
		}
	}

	TestStorageItems.Add({ ItemID, Count });
}

bool AFTHubStorage::RemoveStorageItem(FName ItemID, int32 Count)
{
	if (Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Remove Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	for (int32 Index = 0; Index < TestStorageItems.Num(); ++Index)
	{
		FTStorageItemStruct& StorageItem = TestStorageItems[Index];

		if (StorageItem.ItemID == ItemID)
		{
			if (StorageItem.Count < Count)
			{
				UE_LOG(LogTemp, Warning, TEXT("Remove Failed: %s x%d"), *ItemID.ToString(), Count);
				return false;
			}

			StorageItem.Count -= Count;
			UE_LOG(LogTemp, Warning, TEXT("Remove Success: %s x%d"), *ItemID.ToString(), Count);

			if (StorageItem.Count <= 0)
			{
				TestStorageItems.RemoveAt(Index);
			}

			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Remove Failed: %s x%d"), *ItemID.ToString(), Count);
	return false;
}

int32 AFTHubStorage::GetStorageItemCount(FName ItemID) const
{
	for (const FTStorageItemStruct& StorageItem : TestStorageItems)
	{
		if (StorageItem.ItemID == ItemID)
		{
			return StorageItem.Count;
		}
	}

	return 0;
}

const TArray<FTStorageItemStruct>& AFTHubStorage::GetStorageItems() const
{
	return TestStorageItems;
}

void AFTHubStorage::PrintStorageItems() const
{
	if (TestStorageItems.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Storage Empty"));
		return;
	}

	for (const FTStorageItemStruct& StorageItem : TestStorageItems)
	{
		UE_LOG(LogTemp, Warning, TEXT("Storage: %s x%d"),
			*StorageItem.ItemID.ToString(),
			StorageItem.Count
		);
	}
}
