#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubShopPanelWidget.generated.h"

class UButton;
class UFTInventoryComponent;
class UFTShopSubsystem;
class UFTShopViewModel;
class UImage;
class UTextBlock;
class UTileView;

UCLASS()
class PROJECTFT_API UFTHubShopPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Shop")
	void InitializeShopPanel(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_Items;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_ShopItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_PlayerItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IMG_SelectedItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemTag;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_Tag;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemOwnedCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ItemCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemState;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_TradeQuantity;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_TotalPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_TradeAction;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_BuyMode;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_SellMode;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Buy;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Sell;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuantityMinus;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuantityPlus;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuantityHalf;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_QuantityMax;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_TradeAction;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Refresh;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UTileView* GetPrimaryTileView() const;
	void PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem);
	void HandleItemClicked(UObject* Item);
	void HandleItemSelectionChanged(UObject* Item);

	UFUNCTION()
	void HandleBuyModeClicked();

	UFUNCTION()
	void HandleSellModeClicked();

	UFUNCTION()
	void HandleQuantityMinusClicked();

	UFUNCTION()
	void HandleQuantityPlusClicked();

	UFUNCTION()
	void HandleQuantityHalfClicked();

	UFUNCTION()
	void HandleQuantityMaxClicked();

	UFUNCTION()
	void HandleTradeActionClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UPROPERTY(Transient)
	TObjectPtr<UFTShopViewModel> ViewModel;

	bool bRefreshingFromViewModel = false;
};
