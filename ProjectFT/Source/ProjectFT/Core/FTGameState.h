#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "../Enum/FTFlowStateType.h"
#include "FTGameState.generated.h"

class UFTReportGaugeComponent;

UCLASS()
class PROJECTFT_API AFTGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFTGameState();

	UPROPERTY(BlueprintReadOnly, Category = "FT|Flow")
	EFTFlowStateType CurrentFlowState = EFTFlowStateType::Base;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Security")
	float ReportGauge = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Objective")
	FName CurrentObjectiveId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	TObjectPtr<UFTReportGaugeComponent> ReportGaugeComponent;

	UFUNCTION(BlueprintCallable, Category = "FT|Security")
	void SetReportGauge(float NewReportGauge);

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void SetFlowState(EFTFlowStateType NewFlowState);
};
