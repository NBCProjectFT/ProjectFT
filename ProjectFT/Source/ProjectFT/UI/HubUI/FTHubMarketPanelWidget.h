#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubMarketPanelWidget.generated.h"

class UButton;
class UFTMarketViewModel;
class UFTInventoryComponent;
class UFTShopSubsystem;
class UImage;
class UListView;
class UTextBlock;
class UTileView;

UCLASS()
class PROJECTFT_API UFTHubMarketPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Market")
	void InitializeMarketPanel(UFTShopSubsystem* InShopSubsystem, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UListView* LV_TradePosts;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_BuyRequestsTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_SellOffersTab;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedPostTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedPostDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedPostItem;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedPostPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IMG_SelectedItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemTag;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemOwnedCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_TradeAction;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_SelectedPostItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Trade;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_TradeAction;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	void PopulateListItems(UListView* ListView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem);
	void PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items);
	void HandleTradePostClicked(UObject* Item);

	UFUNCTION()
	void HandleBuyRequestsTabClicked();

	UFUNCTION()
	void HandleSellOffersTabClicked();

	UFUNCTION()
	void HandleTradeClicked();

	UPROPERTY(Transient)
	TObjectPtr<UFTMarketViewModel> ViewModel;

	bool bRefreshingFromViewModel = false;
};
