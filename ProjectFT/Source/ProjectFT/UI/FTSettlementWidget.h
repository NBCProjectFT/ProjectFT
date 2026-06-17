#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTSettlementWidget.generated.h"

UCLASS()
class PROJECTFT_API UFTSettlementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Settlement")
	void ShowCollectedItems(const TArray<FName>& ItemIds);

	UFUNCTION(BlueprintCallable, Category = "FT|Settlement")
	void ShowLostItems(const TArray<FName>& ItemIds);

	UFUNCTION(BlueprintCallable, Category = "FT|Settlement")
	void ShowUnlockedContent(const TArray<FName>& RecipeIds);

	UFUNCTION(BlueprintCallable, Category = "FT|Settlement")
	void ConfirmResult();
};
