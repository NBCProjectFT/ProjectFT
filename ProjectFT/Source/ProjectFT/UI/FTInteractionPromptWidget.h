#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTInteractionPromptWidget.generated.h"

class UTextBlock;

UCLASS()
class PROJECTFT_API UFTInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Interaction")
	void ShowPrompt(const FText& PromptText);

	UFUNCTION(BlueprintCallable, Category = "FT|Interaction")
	void HidePrompt();

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true", BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_InteractionPrompt = nullptr;
};
