#include "FTGameState.h"

#include "ProjectFT/Components/FTReportGaugeComponent.h"
#include "ProjectFT/Components/FTSecurityChaseGaugeComponent.h"

AFTGameState::AFTGameState()
{
	ReportGaugeComponent = CreateDefaultSubobject<UFTReportGaugeComponent>(TEXT("ReportGaugeComponent"));
	SecurityChaseGaugeComponent = CreateDefaultSubobject<UFTSecurityChaseGaugeComponent>(TEXT("SecurityChaseGaugeComponent"));
}

void AFTGameState::SetReportGauge(float NewReportGauge)
{
	ReportGauge = FMath::Clamp(NewReportGauge, 0.0f, 100.0f);
}

void AFTGameState::SetFlowState(EFTFlowStateType NewFlowState)
{
	CurrentFlowState = NewFlowState;
}
