#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTHubCraftTestWidget.generated.h"

class AFTHubWorkbench;
class UButton;
class UFTCraftRecipeListObject;
class UFTInventoryComponent;
class UListView;
class UTextBlock;

UCLASS()
class PROJECTFT_API UFTHubCraftTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hub|Craft")
	void InitializeCraftTest(AFTHubWorkbench* InHubWorkbench, UFTInventoryComponent* InPlayerInventory);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UListView* LV_StorageItems;

	UPROPERTY(meta = (BindWidget))
	UListView* LV_CraftRecipes;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_SelectedRecipeName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_RequiredItems;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_ResultItem;

	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Craft;
	
	UPROPERTY(meta = (BindWidget))
	UButton* BTN_Close;

private:
	void RefreshAll();
	void RefreshStorageItems();
	void RefreshCraftRecipes();
	void UpdateSelectedRecipeDetails();
	void HandleRecipeClicked(UObject* Item);

	UFUNCTION()
	void HandleCraftClicked();

	UPROPERTY(Transient)
	AFTHubWorkbench* HubWorkbench;

	UPROPERTY(Transient)
	UFTInventoryComponent* PlayerInventory;

	UPROPERTY(Transient)
	UFTCraftRecipeListObject* SelectedRecipe;
	
	UFUNCTION()
	void HandleCloseClicked();
};
