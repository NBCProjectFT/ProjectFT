#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubShopWidget.generated.h"

class AFTHubShop;
class UButton;
class UFTInventoryComponent;
class UFTShopItemListObject;
class UListView;
class UTextBlock;

UCLASS()
class PROJECTFT_API UFTHubShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Shop")
	void InitializeShopWidget(AFTHubShop* InHubShop, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UListView* LV_ShopItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemPrice;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemState;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Buy;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Refresh;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Close;

private:
	void RefreshShopItems();
	void UpdateSelectedItemDetails();
	void HandleShopItemClicked(UObject* Item);

	UFUNCTION()
	void HandleBuyClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	AFTHubShop* HubShop;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	UPROPERTY(Transient)
	UFTShopItemListObject* SelectedShopItem;
};
