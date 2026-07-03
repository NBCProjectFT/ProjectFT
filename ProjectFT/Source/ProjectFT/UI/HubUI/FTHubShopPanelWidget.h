#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubShopPanelWidget.generated.h"

class AFTHubShop;
class UButton;
class UFTShopViewModel;
class UFTInventoryComponent;
<<<<<<< Updated upstream
class UFTShopViewModel;
=======
>>>>>>> Stashed changes
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
	UTextBlock* TXT_SelectedItemCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemState;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Buy;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Sell;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Refresh;

private:
	UFUNCTION()
	void RefreshFromViewModel();

<<<<<<< Updated upstream
	void PopulateTileItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem);
=======
	void PopulateItems(UTileView* TileView, const TArray<TObjectPtr<UObject>>& Items, UObject* SelectedItem);
>>>>>>> Stashed changes
	void HandleShopItemClicked(UObject* Item);
	void HandlePlayerItemClicked(UObject* Item);
	void HandleShopItemSelectionChanged(UObject* Item);
	void HandlePlayerItemSelectionChanged(UObject* Item);

	UFUNCTION()
	void HandleBuyClicked();

	UFUNCTION()
	void HandleSellClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UPROPERTY(Transient)
	TObjectPtr<UFTShopViewModel> ViewModel;

<<<<<<< Updated upstream
	bool bRefreshingFromViewModel = false;
=======
	bool bUpdatingSelection = false;
>>>>>>> Stashed changes
};
