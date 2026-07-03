#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTMainMenuWidget.generated.h"

class UButton;
class UUserWidget;

/**
 * Main menu screen widget.
 * WBP_MainMenu owns a reusable WBP_Button named StartButton.
 * This class binds only that widget's inner FTGameButton click, then sends a flow request message.
 */

UCLASS()
class PROJECTFT_API UFTMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UFUNCTION(BlueprintCallable, Category = "FT|MainMenu")
	void HandleStartButtonClicked();

private:
	UButton* ResolveStartButton() const;
	UButton* ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const;
	void BindStartButtonEvents();

	UPROPERTY(Transient)
	TObjectPtr<UButton> CachedStartButton = nullptr;
};
