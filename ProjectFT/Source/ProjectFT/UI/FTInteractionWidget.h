#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTInteractionWidget.generated.h"

UCLASS()
class PROJECTFT_API UFTInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Interaction")
	void ShowPrompt(const FText& PromptText);

	UFUNCTION(BlueprintCallable, Category = "FT|Interaction")
	void HidePrompt();

	UFUNCTION(BlueprintCallable, Category = "FT|Interaction")
	void UpdateProgress(float Progress);
};
