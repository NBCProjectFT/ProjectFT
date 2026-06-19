#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTHubWorkbench.generated.h"

class AFTHubStorage;
class UFTHubCraftTestWidget;
class UDataTable;

UCLASS()
class PROJECTFT_API AFTHubWorkbench : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTHubWorkbench();

	virtual bool Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	bool CanCraftRecipe(const FTCraftRecipeStruct& Recipe) const;
	UFUNCTION(BlueprintCallable, Category = "Craft")
	bool TryCraftRecipe(FName RecipeID);
	
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Craft|UI")
	TSubclassOf<UFTHubCraftTestWidget> HubCraftTestWidgetClass;
	
	

private:
	void OpenCraftWidget(AActor* Interactor);
	void PrintAllRecipes() const;

	const FTCraftRecipeStruct* FindRecipeByID(FName RecipeID) const;

	UPROPERTY(Transient)
	UFTHubCraftTestWidget* HubCraftTestWidget;
	
	
};