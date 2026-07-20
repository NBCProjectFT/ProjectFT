#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTShelfHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class PROJECTFT_API UFTShelfHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Shelf")
	void SetHealthPercent(float Percent);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_HealthBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_HealthPercent;
};
