#include "FTInteractionWidget.h"

void UFTInteractionWidget::ShowPrompt(const FText& PromptText)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UFTInteractionWidget::HidePrompt()
{
	SetVisibility(ESlateVisibility::Hidden);
}

void UFTInteractionWidget::UpdateProgress(float Progress)
{
}
