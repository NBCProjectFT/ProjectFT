#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTShopViewModel.generated.h"

class UFTInventoryComponent;
class UFTItemTileListObject;
class UFTShopSubsystem;
class UTexture2D;

UENUM(BlueprintType)
enum class EFTShopSelectionSource : uint8
{
	None,
	Shop,
	Player
};

UENUM(BlueprintType)
enum class EFTShopPanelMode : uint8
{
	Buy,
	Sell
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
	const TArray<TObjectPtr<UObject>>& GetCurrentItemObjects() const;
	UFTItemTileListObject* GetSelectedShopItemObject() const;
	UFTItemTileListObject* GetSelectedPlayerItemObject() const;
	UFTItemTileListObject* GetSelectedCurrentItemObject() const;

	FText GetSelectedItemNameText() const;
	FText GetSelectedItemTagText() const;
	FText GetSelectedItemDescriptionText() const;
	FText GetSelectedItemPriceText() const;
	FText GetSelectedItemCountText() const;
	FText GetSelectedItemOwnedCountText() const;
	FText GetTradeQuantityText() const;
	FText GetTradeTotalPriceText() const;
	FText GetTradeActionText() const;
	FText GetSelectedItemStateText() const;
	TSoftObjectPtr<UTexture2D> GetSelectedItemIcon() const;
	bool IsBuyMode() const;
	bool IsSellMode() const;
	bool CanBuySelectedItem() const;
	bool CanSellSelectedItem() const;
	bool CanExecuteTradeAction() const;

	void RefreshAll();
	void SetBuyMode();
	void SetSellMode();
	void SelectShopItemObject(UObject* ItemObject);
	void SelectPlayerItemObject(UObject* ItemObject);
	void SelectCurrentItemObject(UObject* ItemObject);
	void IncreaseTradeQuantity();
	void DecreaseTradeQuantity();
	bool BuySelectedItem();
	bool SellSelectedItem();
	bool ExecuteTradeAction();
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
	int32 GetMaxTradeQuantity() const;
	int32 GetUnitPrice() const;
	void ResetTradeQuantity();
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
	EFTShopPanelMode CurrentMode = EFTShopPanelMode::Buy;
	int32 TradeQuantity = 1;
};
