#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTInventoryWidget.generated.h"

UCLASS()
class PROJECTFT_API UFTInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void RefreshItemList();

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void UpdateWeight(float CurrentWeight, float MaxWeight);

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void ShowItemDetail(FName ItemId);
};
