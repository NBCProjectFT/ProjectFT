#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTHubWorkbench.generated.h"

class AFTHubStorage;
class UDataTable;
class UFTInventoryComponent;
class UFTStorageSubsystem;

UCLASS()
class PROJECTFT_API AFTHubWorkbench : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubWorkbench();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	bool CanCraftRecipe(const FTCraftRecipeStruct& Recipe, UFTInventoryComponent* PlayerInventory) const;
	UFUNCTION(BlueprintCallable, Category = "Craft")
	bool TryCraftRecipe(FName RecipeID, UFTInventoryComponent* PlayerInventory);
	
	UFUNCTION(BlueprintCallable, Category = "Craft|UI")
	void CloseCraftWidget();

	void GetCraftRecipes(TArray<FTCraftRecipeStruct>& OutRecipes) const;

	AFTHubStorage* GetHubStorage() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	UDataTable* CraftRecipeDataTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Craft")
	AFTHubStorage* HubStorage;

private:
	void OpenCraftWidget(AActor* Interactor);

	const FTCraftRecipeStruct* FindRecipeByID(FName RecipeID) const;
	UFTStorageSubsystem* GetStorageSubsystem() const;
	UFTInventoryComponent* GetStorageInventory() const;
	int32 GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const;
	bool ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count);
};
