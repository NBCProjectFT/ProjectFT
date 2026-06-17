#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTHUDViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTHUDViewModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD")
	float HP = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD")
	float Stamina = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD")
	float CurrentWeight = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD")
	float MaxWeight = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD")
	float ReportGauge = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FT|HUD")
	FText ObjectiveText;

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void NotifyChanged();
};
