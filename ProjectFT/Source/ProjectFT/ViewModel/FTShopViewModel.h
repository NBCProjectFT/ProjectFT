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

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Items")
	TArray<UObject*> GetShopItemObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Items")
	TArray<UObject*> GetPlayerItemObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Items")
	TArray<UObject*> GetCurrentItemObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Selection")
	UFTItemTileListObject* GetSelectedShopItemObject() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Selection")
	UFTItemTileListObject* GetSelectedPlayerItemObject() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Selection")
	UFTItemTileListObject* GetSelectedCurrentItemObject() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Data")
	bool HasSelectedItem() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Data")
	int32 GetSelectedItemUnitPrice() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Data")
	int32 GetSelectedItemOwnedCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Data")
	int32 GetTradeQuantity() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Data")
	int32 GetTradeTotalPrice() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Presentation")
	FText GetSelectedItemNameText() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Presentation")
	FText GetSelectedItemDescriptionText() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Presentation")
	TSoftObjectPtr<UTexture2D> GetSelectedItemIcon() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|State")
	bool IsBuyMode() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|State")
	bool IsSellMode() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Rules")
	bool CanBuySelectedItem() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Rules")
	bool CanSellSelectedItem() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Rules")
	bool CanExecuteTradeAction() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Rules")
	bool CanSetTradeQuantityToHalf() const;

	UFUNCTION(BlueprintPure, Category = "FT|Shop|Rules")
	bool CanSetTradeQuantityToMax() const;

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
	void RefreshAll();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Mode")
	void SetBuyMode();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Mode")
	void SetSellMode();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Selection")
	void SelectShopItemObject(UObject* ItemObject);

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Selection")
	void SelectPlayerItemObject(UObject* ItemObject);

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Selection")
	void SelectCurrentItemObject(UObject* ItemObject);

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Quantity")
	void IncreaseTradeQuantity();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Quantity")
	void DecreaseTradeQuantity();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Quantity")
	void SetTradeQuantityToHalf();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Quantity")
	void SetTradeQuantityToMax();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Trade")
	bool BuySelectedItem();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Trade")
	bool SellSelectedItem();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop|Trade")
	bool ExecuteTradeAction();

	UFUNCTION(BlueprintCallable, Category = "FT|Shop")
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
	bool bTransactionInProgress = false;
};
