#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubStorageWidget.generated.h"

class AFTHubStorage;
class UButton;
class UFTInventoryComponent;
class UListView;
class USpinBox;
class UTextBlock;

UCLASS()
class PROJECTFT_API UFTHubStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Storage")
	void InitializeStorageWidget(AFTHubStorage* InHubStorage, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UListView* LV_PlayerItems;

	UPROPERTY(meta = (BindWidget))
	UListView* LV_StorageItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedItem;

	UPROPERTY(meta = (BindWidgetOptional))
	USpinBox* SPB_MoveCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Store;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BTN_Take;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Close;

private:
	enum class EStorageTransferSourceType : uint8
	{
		None,
		Player,
		Storage
	};

	void RefreshAllItems();
	void RefreshPlayerItems();
	void RefreshStorageItems();
	void UpdateTransferControls();
	int32 GetRequestedCount() const;
	int32 GetSelectedItemCount() const;

	void HandlePlayerItemClicked(UObject* Item);
	void HandleStorageItemClicked(UObject* Item);

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleStoreClicked();

	UFUNCTION()
	void HandleTakeClicked();

	UPROPERTY(Transient)
	AFTHubStorage* HubStorage;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	FName SelectedItemID = NAME_None;
	EStorageTransferSourceType SelectedSource = EStorageTransferSourceType::None;
};
