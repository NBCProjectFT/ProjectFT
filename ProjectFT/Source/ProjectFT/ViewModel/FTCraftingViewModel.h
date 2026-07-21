#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTCraftingViewModel.generated.h"

class UFTCraftingSubsystem;
class UFTInventoryComponent;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTCraftingViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTCraftingViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(
		UFTCraftingSubsystem* InCraftingSubsystem,
		UFTInventoryComponent* InPlayerInventory,
		UFTInventoryComponent* InStorageInventory);

	UFUNCTION(BlueprintPure, Category = "FT|Crafting|Items")
	TArray<UObject*> GetRecipeObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Crafting|Items")
	TArray<UObject*> GetRequiredItemObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Crafting|Presentation")
	FText GetSelectedRecipeNameText() const;
	UFUNCTION(BlueprintPure, Category = "FT|Crafting|Presentation")
	FText GetSelectedRecipeDescriptionText() const;
	UFUNCTION(BlueprintPure, Category = "FT|Crafting|Presentation")
	TSoftObjectPtr<UTexture2D> GetResultItemIcon() const;
	UFUNCTION(BlueprintPure, Category = "FT|Crafting|Rules")
	bool CanCraftSelectedRecipe() const;

	UFUNCTION(BlueprintCallable, Category = "FT|Crafting|Filter")
	void SetCraftableOnly(bool bInCraftableOnly);
	UFUNCTION(BlueprintCallable, Category = "FT|Crafting|Selection")
	void SelectRecipeObject(UObject* RecipeObject);
	UFUNCTION(BlueprintCallable, Category = "FT|Crafting|Craft")
	bool CraftSelectedRecipe();

	UPROPERTY(BlueprintAssignable, Category = "FT|Crafting")
	FFTCraftingViewModelChanged OnChanged;

private:
	void RefreshAll();
	void NotifyChanged();

	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshRecipes();
	void RefreshSelectedRecipeDetails();
	void RefreshRequiredItemObjects();
	void BindInventoryDelegates();
	void UnbindInventoryDelegates();
	bool ShouldShowRecipe(bool bRecipeCanCraft) const;
	int32 GetOwnedIngredientCount(FName ItemID) const;
	void ClearSelectedRecipeDetails();

	UPROPERTY(Transient)
	TObjectPtr<UFTCraftingSubsystem> CraftingSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> StorageInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RecipeObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RequiredItemObjects;

	UPROPERTY(Transient)
	TObjectPtr<class UFTCraftRecipeListObject> SelectedRecipeObject;

	TSoftObjectPtr<UTexture2D> ResultItemIcon;

	FText SelectedRecipeNameText;
	FText SelectedRecipeDescriptionText;
	bool bCraftableOnly = false;
	bool bTransactionInProgress = false;
};
