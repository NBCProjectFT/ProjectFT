#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubMarketPanelWidget.generated.h"

class AFTHubShop;
class UButton;
class UFTInventoryComponent;
class UFTItemTileListObject;
class UFTTradePostListObject;
class UListView;
class UTextBlock;
class UTileView;

UCLASS()
class PROJECTFT_API UFTHubMarketPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Market")
	void InitializeMarketPanel(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
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
	UTileView* TV_SelectedPostItems;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Trade;

private:
	void RefreshTradePosts();
	void UpdateSelectedPostDetails();
	void RefreshSelectedPostItems();
	void SetBuyRequestMode(bool bInBuyRequestMode);
	void HandleTradePostClicked(UObject* Item);

	UFUNCTION()
	void HandleBuyRequestsTabClicked();

	UFUNCTION()
	void HandleSellOffersTabClicked();

	UFUNCTION()
	void HandleTradeClicked();

	UPROPERTY(Transient)
	AFTHubShop* HubShop;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	UPROPERTY(Transient)
	UFTTradePostListObject* SelectedPost;

	bool bBuyRequestMode = true;
};
