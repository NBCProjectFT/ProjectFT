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

	UFUNCTION(BlueprintPure, Category = "Hub|Shop")
	UFTShopViewModel* GetShopViewModel() const { return ViewModel; }

	/**
	 * Blueprint presentation hook. Implement this in WBP_FTHubShopPanelWidget and
	 * read every visible value from the supplied ViewModel.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Hub|Shop", meta = (DisplayName = "On Shop ViewModel Changed"))
	void BP_OnShopViewModelChanged(UFTShopViewModel* ShopViewModel);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTileView* TV_Items;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* IMG_SelectedItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemTag;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemDescription;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemOwnedCount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemPrice;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemCount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItemState;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_TradeQuantity;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_TotalPrice;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* TXT_TradeAction;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* BTN_BuyMode;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* BTN_SellMode;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* BTN_QuantityMinus;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* BTN_QuantityPlus;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* BTN_QuantityHalf;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* BTN_QuantityMax;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* BTN_TradeAction;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UPROPERTY(Transient)
	TObjectPtr<UFTShopViewModel> ViewModel;
};
