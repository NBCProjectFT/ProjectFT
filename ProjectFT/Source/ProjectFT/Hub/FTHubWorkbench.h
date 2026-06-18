// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"
#include "FTHubWorkbench.generated.h"

class AFTHubStorage;
class UFTHubCraftTestWidget;

UCLASS()
class PROJECTFT_API AFTHubWorkbench : public AActor
{
	GENERATED_BODY()

public:
	AFTHubWorkbench();
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact();

	bool CanCraftRecipe(const FTCraftRecipeStruct& Recipe) const;

	UFUNCTION(BlueprintCallable, Category = "Craft")
	bool TryCraftRecipe(FName RecipeID);

	void GetCraftRecipes(TArray<FTCraftRecipeStruct>& OutRecipes) const;

	AFTHubStorage* GetHubStorage() const;

protected:

	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Craft")
	UDataTable* CraftRecipeDataTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Craft")
	AFTHubStorage* HubStorage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Craft|Test")
	TSubclassOf<UFTHubCraftTestWidget> HubCraftTestWidgetClass;

	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

private:
	void PrintAllRecipes() const;

	const FTCraftRecipeStruct* FindRecipeByID(FName RecipeID) const;

	UPROPERTY(Transient)
	UFTHubCraftTestWidget* HubCraftTestWidget;
};
