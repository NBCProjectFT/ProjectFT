#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTCraftingSubsystem.generated.h"

class UDataTable;
class UFTInventoryComponent;

/**
 * 허브 제작의 레시피 조회와 제작 가능 여부 판정을 담당한다.
 * 월드 액터를 보관하지 않고, 판정에 필요한 인벤토리를 호출 시점에 전달받는다.
 */
UCLASS()
class PROJECTFT_API UFTCraftingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "FT|Crafting")
	bool FindRecipe(FName RecipeID, FTCraftRecipeStruct& OutRecipe) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Crafting")
	void GetCraftRecipes(TArray<FTCraftRecipeStruct>& OutRecipes) const;

	UFUNCTION(BlueprintPure, Category = "FT|Crafting")
	bool IsRecipeUnlocked(FName RecipeID) const;

	UFUNCTION(BlueprintPure, Category = "FT|Crafting")
	int32 GetCombinedItemCount(
		const UFTInventoryComponent* PlayerInventory,
		const UFTInventoryComponent* StorageInventory,
		FName ItemID) const;

	UFUNCTION(BlueprintPure, Category = "FT|Crafting")
	bool CanCraftRecipe(
		const FTCraftRecipeStruct& Recipe,
		const UFTInventoryComponent* PlayerInventory,
		const UFTInventoryComponent* StorageInventory) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Crafting")
	bool TryCraftRecipe(
		FName RecipeID,
		UFTInventoryComponent* PlayerInventory,
		UFTInventoryComponent* StorageInventory);

private:
	void EnsureRecipeDataLoaded() const;

	UPROPERTY(Transient)
	mutable TObjectPtr<UDataTable> CraftRecipeDataTable = nullptr;
};
