#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTCraftingViewModel.generated.h"

class AFTHubWorkbench;
class UFTInventoryComponent;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTCraftingViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTCraftingViewModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "FT|Crafting")
	TArray<FName> RecipeList;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Crafting")
	FName SelectedRecipe = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Crafting")
	bool bCanCraft = false;

	void Initialize(AFTHubWorkbench* InHubWorkbench, UFTInventoryComponent* InPlayerInventory);

	const TArray<TObjectPtr<UObject>>& GetStorageItemObjects() const;
	const TArray<TObjectPtr<UObject>>& GetRecipeObjects() const;
	const TArray<TObjectPtr<UObject>>& GetRequiredItemObjects() const;

	FText GetRecipeCountText() const;
	FText GetSelectedRecipeNameText() const;
	FText GetSelectedRecipeTierText() const;
	FText GetSelectedRecipeDescriptionText() const;
	FText GetCraftTimeText() const;
	FText GetCraftAmountText() const;
	FText GetRequiredItemsText() const;
	FText GetResultItemText() const;
	UTexture2D* GetResultItemIcon() const;
	bool CanCraftSelectedRecipe() const;

	void RefreshAll();
	void SetCraftableOnly(bool bInCraftableOnly);
	void SetSearchText(const FText& InSearchText);
	void SelectRecipeObject(UObject* RecipeObject);
	bool CraftSelectedRecipe();

	UFUNCTION(BlueprintCallable, Category = "FT|Crafting")
	void NotifyChanged();

	UPROPERTY(BlueprintAssignable, Category = "FT|Crafting")
	FFTCraftingViewModelChanged OnChanged;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshStorageItems();
	void RefreshRecipes();
	void RefreshSelectedRecipeDetails();
	void RefreshRequiredItemObjects();
	void BindInventoryDelegates();
	void UnbindInventoryDelegates();
	bool ShouldShowRecipe(const FTCraftRecipeStruct& Recipe, bool bRecipeCanCraft) const;
	int32 GetOwnedIngredientCount(FName ItemID) const;
	const class UFTItemDataAsset* FindItemData(FName ItemID) const;
	void ClearSelectedRecipeDetails();

	UPROPERTY(Transient)
	TObjectPtr<AFTHubWorkbench> HubWorkbench;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> StorageItemObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RecipeObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RequiredItemObjects;

	UPROPERTY(Transient)
	TObjectPtr<class UFTCraftRecipeListObject> SelectedRecipeObject;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ResultItemIcon;

	FText RecipeCountText;
	FText SelectedRecipeNameText;
	FText SelectedRecipeTierText;
	FText SelectedRecipeDescriptionText;
	FText CraftTimeText;
	FText CraftAmountText;
	FText RequiredItemsText;
	FText ResultItemText;
	FText SearchText;
	bool bCraftableOnly = false;
};
