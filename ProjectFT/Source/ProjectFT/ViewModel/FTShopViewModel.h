#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTShopViewModel.generated.h"

class UFTInventoryComponent;
class UFTItemTileListObject;
class UFTShopSubsystem;

UENUM(BlueprintType)
enum class EFTShopSelectionSource : uint8
{
	None,
	Shop,
	Player
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTShopViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTShopViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory);

	const TArray<TObjectPtr<UObject>>& GetShopItemObjects() const;
	const TArray<TObjectPtr<UObject>>& GetPlayerItemObjects() const;
	UFTItemTileListObject* GetSelectedShopItemObject() const;
	UFTItemTileListObject* GetSelectedPlayerItemObject() const;

	FText GetSelectedItemNameText() const;
	FText GetSelectedItemDescriptionText() const;
	FText GetSelectedItemPriceText() const;
	FText GetSelectedItemCountText() const;
	FText GetSelectedItemStateText() const;
	bool CanBuySelectedItem() const;
	bool CanSellSelectedItem() const;

	void RefreshAll();
	void SelectShopItemObject(UObject* ItemObject);
	void SelectPlayerItemObject(UObject* ItemObject);
	bool BuySelectedItem();
	bool SellSelectedItem();
	void RefreshShop();

	UPROPERTY(BlueprintAssignable, Category = "FT|Shop")
	FFTShopViewModelChanged OnChanged;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshShopItems();
	void RefreshPlayerItems();
	void RestoreSelection(FName PreviousShopItemID, FName PreviousPlayerItemID, EFTShopSelectionSource PreviousSource);
	void UpdateSelectionChecks();
	void BindInventoryDelegate();
	void UnbindInventoryDelegate();
	const UFTItemTileListObject* GetSelectedItem() const;
	FName GetSelectedItemID() const;
	void ClearSelection();
	void NotifyChanged();

	UPROPERTY(Transient)
	TObjectPtr<UFTShopSubsystem> ShopSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> ShopItemObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> PlayerItemObjects;

	UPROPERTY(Transient)
	TObjectPtr<UFTItemTileListObject> SelectedShopItem;

	UPROPERTY(Transient)
	TObjectPtr<UFTItemTileListObject> SelectedPlayerItem;

	EFTShopSelectionSource SelectedSource = EFTShopSelectionSource::None;
};
