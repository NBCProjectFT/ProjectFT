#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTHubCraftTestWidget.generated.h"

class UButton;
class UCheckBox;
class UEditableTextBox;
class UFTCraftingViewModel;
class UFTInventoryComponent;
class UImage;
class UListView;
class UTextBlock;
class UTileView;

UCLASS()
class PROJECTFT_API UFTHubCraftTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Craft")
	void InitializeCraftTest(
		UFTInventoryComponent* InPlayerInventory,
		UFTInventoryComponent* InStorageInventory,
		UFTCraftingViewModel* InViewModel);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UListView* LV_CraftRecipes;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_StorageItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTileView* TV_RequiredItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox* CHK_CraftableOnly;

	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox* CHK_ShowCraftableOnly;

	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox* EDT_SearchRecipe;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_RecipeCount;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IMG_ResultItemIcon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_SelectedRecipeName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedRecipeTier;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_SelectedRecipeDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_CraftTime;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_CraftAmount;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_RequiredItems;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TXT_ResultItem;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Craft;
	
	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Close;

private:
	UFUNCTION()
	void RefreshFromViewModel();

	UListView* GetStorageItemsView() const;
	UCheckBox* GetCraftableOnlyCheckBox() const;
	void PopulateItems(UListView* ItemsView, const TArray<TObjectPtr<UObject>>& Items);
	void HandleRecipeClicked(UObject* Item);

	UFUNCTION()
	void HandleCraftClicked();

	UFUNCTION()
	void HandleCraftableOnlyChanged(bool bIsChecked);

	UFUNCTION()
	void HandleSearchRecipeTextChanged(const FText& Text);

	UPROPERTY(Transient)
	TObjectPtr<UFTCraftingViewModel> ViewModel;
	
	UFUNCTION()
	void HandleCloseClicked();
};
