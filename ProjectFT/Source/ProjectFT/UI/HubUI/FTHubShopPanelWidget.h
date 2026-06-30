#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubShopPanelWidget.generated.h"

class AFTHubShop;
class UButton;
class UFTInventoryComponent;
class UFTItemTileListObject;
class UTextBlock;
class UTileView;

UCLASS()
class PROJECTFT_API UFTHubShopPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Shop")
	void InitializeShopPanel(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UTileView* TV_ShopItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_PlayerItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemState;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Buy;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Sell;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Refresh;

private:
	enum class EShopSelectionSourceType : uint8
	{
		None,
		Shop,
		Player
	};

	void RefreshShopItems();
	void RefreshPlayerItems();
	void RefreshAllItems();
	void UpdateSelectedItemDetails();
	void HandleShopItemClicked(UObject* Item);
	void HandlePlayerItemClicked(UObject* Item);

	UFUNCTION()
	void HandleBuyClicked();

	UFUNCTION()
	void HandleSellClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UPROPERTY(Transient)
	AFTHubShop* HubShop;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	UPROPERTY(Transient)
	UFTItemTileListObject* SelectedShopItem;

	UPROPERTY(Transient)
	UFTItemTileListObject* SelectedPlayerItem;

	EShopSelectionSourceType SelectedSource = EShopSelectionSourceType::None;
};
