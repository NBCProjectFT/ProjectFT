#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTSettlementViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTSettlementViewModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "FT|Settlement")
	TArray<FName> CollectedItems;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Settlement")
	TArray<FName> LostItems;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Settlement")
	TArray<FName> UnlockedRecipes;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Settlement")
	FText RewardText;

	UFUNCTION(BlueprintCallable, Category = "FT|Settlement")
	void NotifyChanged();
};
