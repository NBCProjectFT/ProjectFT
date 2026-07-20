#include "FTShelfHealthBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UFTShelfHealthBarWidget::SetHealthPercent(float Percent)
{
	const float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);

	if (PB_HealthBar)
	{
		PB_HealthBar->SetPercent(ClampedPercent);
	}

	if (TXT_HealthPercent)
	{
		TXT_HealthPercent->SetText(FText::AsPercent(ClampedPercent));
	}
}
