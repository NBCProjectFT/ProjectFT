#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTMainHUDWidget.generated.h"

UCLASS()
class PROJECTFT_API UFTMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateHP(float NewHP);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateStamina(float NewStamina);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateWeight(float CurrentWeight, float MaxWeight);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateReportGauge(float NewReportGauge);

	UFUNCTION(BlueprintCallable, Category = "FT|HUD")
	void UpdateObjective(const FText& NewObjectiveText);
};
