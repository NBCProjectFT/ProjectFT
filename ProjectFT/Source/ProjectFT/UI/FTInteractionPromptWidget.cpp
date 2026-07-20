#include "FTInteractionPromptWidget.h"

#include "Components/TextBlock.h"

void UFTInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	HidePrompt();
}

void UFTInteractionPromptWidget::ShowPrompt(const FText& PromptText)
{
	if (TXT_InteractionPrompt)
	{
		TXT_InteractionPrompt->SetText(PromptText);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UFTInteractionPromptWidget::HidePrompt()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
