#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubStorageWidget.generated.h"

class AFTHubStorage;
class UButton;
class UFTHubStorageViewModel;
class UFTInventoryComponent;
class UListView;
class USpinBox;
class UTextBlock;
class UTileView;

UCLASS()
class PROJECTFT_API UFTHubStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Storage")
	void InitializeStorageWidget(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory, UFTHubStorageViewModel* InViewModel);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_PlayerItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_StorageItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItem;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_PlayerWeight;

	UPROPERTY(meta = (BindWidgetOptional))
	USpinBox* SPB_MoveCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Store;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Take;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StoreAll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_TakeAll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_PlayerFilterAll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_PlayerFilterWeapon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_PlayerFilterHealing;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_PlayerFilterCommon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StorageFilterAll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StorageFilterWeapon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StorageFilterHealing;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_StorageFilterCommon;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Close;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UListView* GetPlayerItemsView() const;
	UListView* GetStorageItemsView() const;
	void PopulateItems(UListView* ItemsView, const TArray<TObjectPtr<UObject>>& Items);
	void PushSelectedItemsToViewModel(UListView* ItemsView, bool bFromPlayerItems);

	void HandlePlayerItemClicked(UObject* Item);
	void HandleStorageItemClicked(UObject* Item);
	void HandlePlayerItemSelectionChanged(UObject* Item);
	void HandleStorageItemSelectionChanged(UObject* Item);

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleStoreClicked();

	UFUNCTION()
	void HandleTakeClicked();

	UFUNCTION()
	void HandleStoreAllClicked();

	UFUNCTION()
	void HandleTakeAllClicked();

	UFUNCTION()
	void HandlePlayerFilterAllClicked();

	UFUNCTION()
	void HandlePlayerFilterWeaponClicked();

	UFUNCTION()
	void HandlePlayerFilterHealingClicked();

	UFUNCTION()
	void HandlePlayerFilterCommonClicked();

	UFUNCTION()
	void HandleStorageFilterAllClicked();

	UFUNCTION()
	void HandleStorageFilterWeaponClicked();

	UFUNCTION()
	void HandleStorageFilterHealingClicked();

	UFUNCTION()
	void HandleStorageFilterCommonClicked();

	UPROPERTY(Transient)
	TObjectPtr<AFTHubStorage> HubStorage;

	UPROPERTY(Transient)
	TObjectPtr<UFTHubStorageViewModel> ViewModel;

	bool bRefreshingFromViewModel = false;
};
