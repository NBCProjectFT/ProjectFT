#include "FTGameState.h"

void AFTGameState::SetReportGauge(float NewReportGauge)
{
	ReportGauge = FMath::Clamp(NewReportGauge, 0.0f, 100.0f);
}

void AFTGameState::SetFlowState(EFTFlowStateType NewFlowState)
{
	CurrentFlowState = NewFlowState;
}
