#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTShopViewModel.generated.h"

class AFTHubShop;
class UFTInventoryComponent;
class UFTItemTileListObject;

<<<<<<< Updated upstream
=======
UENUM(BlueprintType)
enum class EFTShopSelectionSource : uint8
{
	None,
	Shop,
	Player
};

>>>>>>> Stashed changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTShopViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTShopViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory);

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
<<<<<<< Updated upstream
	void RefreshShopStock();
=======
	void RefreshShop();
>>>>>>> Stashed changes

	UPROPERTY(BlueprintAssignable, Category = "FT|Shop")
	FFTShopViewModelChanged OnChanged;

private:
<<<<<<< Updated upstream
	enum class EShopSelectionSourceType : uint8
	{
		None,
		Shop,
		Player
	};

=======
>>>>>>> Stashed changes
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshShopItems();
	void RefreshPlayerItems();
<<<<<<< Updated upstream
	void RestoreSelection(FName PreviousShopItemID, FName PreviousPlayerItemID, EShopSelectionSourceType PreviousSource);
	void ClearSelection();
	void ClearItemChecks();
	void BindInventoryDelegate();
	void UnbindInventoryDelegate();
	UFTItemTileListObject* GetSelectedItemObject() const;
=======
	void RestoreSelection(FName PreviousShopItemID, FName PreviousPlayerItemID);
	void UpdateSelectionChecks();
	void BindInventoryDelegate();
	void UnbindInventoryDelegate();
	const UFTItemTileListObject* GetSelectedItem() const;
	FName GetSelectedItemID() const;
	void ClearSelection();
>>>>>>> Stashed changes
	void NotifyChanged();

	UPROPERTY(Transient)
	TObjectPtr<AFTHubShop> HubShop;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> ShopItemObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> PlayerItemObjects;

	UPROPERTY(Transient)
<<<<<<< Updated upstream
	TObjectPtr<UFTItemTileListObject> SelectedShopItemObject;

	UPROPERTY(Transient)
	TObjectPtr<UFTItemTileListObject> SelectedPlayerItemObject;

	EShopSelectionSourceType SelectedSource = EShopSelectionSourceType::None;
=======
	TObjectPtr<UFTItemTileListObject> SelectedShopItem;

	UPROPERTY(Transient)
	TObjectPtr<UFTItemTileListObject> SelectedPlayerItem;

	EFTShopSelectionSource SelectedSource = EFTShopSelectionSource::None;
>>>>>>> Stashed changes
};
