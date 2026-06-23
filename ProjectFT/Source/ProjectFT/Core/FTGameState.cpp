#include "FTGameState.h"

#include "ProjectFT/Components/FTReportGaugeComponent.h"

AFTGameState::AFTGameState()
{
	ReportGaugeComponent = CreateDefaultSubobject<UFTReportGaugeComponent>(TEXT("ReportGaugeComponent"));
}

void AFTGameState::SetReportGauge(float NewReportGauge)
{
	ReportGauge = FMath::Clamp(NewReportGauge, 0.0f, 100.0f);
}

void AFTGameState::SetFlowState(EFTFlowStateType NewFlowState)
{
	CurrentFlowState = NewFlowState;
}
