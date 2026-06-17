#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTInventoryViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTInventoryViewModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	TArray<FName> InventoryItems;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	float CurrentWeight = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	float MaxWeight = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Inventory")
	FName SelectedItem = NAME_None;

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void NotifyChanged();
};
