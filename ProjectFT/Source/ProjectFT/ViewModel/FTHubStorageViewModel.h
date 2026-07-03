#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Struct/FTStorageItemStruct.h"
#include "FTHubStorageViewModel.generated.h"

class AFTHubStorage;
class UFTInventoryComponent;

UENUM(BlueprintType)
enum class EFTHubStorageTransferSource : uint8
{
	None,
	Player,
	Storage
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTHubStorageViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTHubStorageViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory, bool bInUsePlayerTileItems, bool bInUseStorageTileItems);

	const TArray<TObjectPtr<UObject>>& GetPlayerItemObjects() const;
	const TArray<TObjectPtr<UObject>>& GetStorageItemObjects() const;

	FText GetSelectedItemText() const;
	FText GetPlayerWeightText() const;
	int32 GetSelectedEntryCount() const;
	int32 GetPlayerSelectedEntryCount() const;
	int32 GetStorageSelectedEntryCount() const;
	bool CanStoreSelected() const;
	bool CanTakeSelected() const;
	bool CanStoreAll() const;
	bool CanTakeAll() const;

	void RefreshAll();
	void SetSelectedItems(EFTHubStorageTransferSource SourceType, const TArray<UObject*>& Items);
	void ClearSelection();
	void SetPlayerFilter(EFTItemCategoryType FilterCategory);
	void SetStorageFilter(EFTItemCategoryType FilterCategory);
	bool StoreSelectedItems();
	bool TakeSelectedItems();
	bool StoreAllItems();
	bool TakeAllItems();

	UPROPERTY(BlueprintAssignable, Category = "Hub|Storage")
	FFTHubStorageViewModelChanged OnChanged;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshPlayerItems();
	void RefreshStorageItems();
	void BindInventoryDelegates();
	void UnbindInventoryDelegates();
	bool ShouldShowItem(FName ItemID, EFTItemCategoryType FilterCategory) const;
	EFTItemCategoryType GetItemCategory(FName ItemID) const;
	bool TryReadItemObject(UObject* ItemObject, FTStorageItemStruct& OutItem) const;
	bool TransferSelectedItems(EFTHubStorageTransferSource SourceType);
	bool TransferAllItems(EFTHubStorageTransferSource SourceType);

	UPROPERTY(Transient)
	TObjectPtr<AFTHubStorage> HubStorage;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> PlayerItemObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> StorageItemObjects;

	TArray<FTStorageItemStruct> SelectedItems;
	EFTHubStorageTransferSource SelectedSource = EFTHubStorageTransferSource::None;
	EFTItemCategoryType PlayerFilterCategory = EFTItemCategoryType::None;
	EFTItemCategoryType StorageFilterCategory = EFTItemCategoryType::None;
	bool bUsePlayerTileItems = false;
	bool bUseStorageTileItems = false;
};
