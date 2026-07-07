#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "FTStorageSubsystem.generated.h"

class UFTInventoryComponent;

UCLASS()
class PROJECTFT_API UFTStorageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void InitializeStorage(UFTInventoryComponent* StorageInventory, const TArray<FTStorageItemStruct>& InitialItems) const;

	bool AddStorageItem(UFTInventoryComponent* StorageInventory, FName ItemID, int32 Count) const;
	bool RemoveStorageItem(UFTInventoryComponent* StorageInventory, FName ItemID, int32 Count) const;
	int32 GetStorageItemCount(const UFTInventoryComponent* StorageInventory, FName ItemID) const;
	void GetStorageItems(const UFTInventoryComponent* StorageInventory, TArray<FTStorageItemStruct>& OutItems) const;

	bool StoreItemFromInventory(UFTInventoryComponent* StorageInventory, UFTInventoryComponent* SourceInventory, FName ItemID, int32 Count) const;
	bool TakeItemToInventory(UFTInventoryComponent* StorageInventory, UFTInventoryComponent* TargetInventory, FName ItemID, int32 Count) const;

	int32 GetCombinedItemCount(const UFTInventoryComponent* PlayerInventory, const UFTInventoryComponent* StorageInventory, FName ItemID) const;
	bool ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, UFTInventoryComponent* StorageInventory, FName ItemID, int32 Count) const;

	void PrintStorageItems(const UFTInventoryComponent* StorageInventory) const;
};
