#include "FTInteractionStatusWidget.h"

#include "Components/WidgetSwitcher.h"
#include "FTInteractionPromptWidget.h"
#include "FTProgressWidget.h"
#include "GameFramework/Pawn.h"
#include "ProjectFT/Components/FTCaptureEscapeComponent.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Interface/FTInteractable.h"

void UFTInteractionStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.0f);
	SetWidgetOptionalVisibility(WBP_Progress, false);
	if (WBP_InteractionPrompt)
	{
		WBP_InteractionPrompt->HidePrompt();
	}
}

void UFTInteractionStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UFTInteractionComponent* InteractionComponent = ResolveInteractionComponent();
	UFTCaptureEscapeComponent* CaptureEscapeComponent = ResolveCaptureEscapeComponent();
	const bool bIsEscaping = CaptureEscapeComponent && CaptureEscapeComponent->IsCaptured();
	const bool bIsChanneling = InteractionComponent && InteractionComponent->IsChanneling();
	const bool bHasPromptTarget = InteractionComponent && InteractionComponent->GetFocusedActor();

	if (bIsEscaping)
	{
		SetStatusMode(EFTInteractionStatusModeType::Escape);
		return;
	}

	if (bIsChanneling)
	{
		SetStatusMode(EFTInteractionStatusModeType::ShelfSearch);
		return;
	}

	if (bHasPromptTarget)
	{
		SetStatusMode(EFTInteractionStatusModeType::Prompt);
		UpdatePrompt(InteractionComponent);
		return;
	}

	SetStatusMode(EFTInteractionStatusModeType::None);
}

UFTInteractionComponent* UFTInteractionStatusWidget::ResolveInteractionComponent()
{
	if (CachedInteractionComponent)
	{
		return CachedInteractionComponent;
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return nullptr;
	}

	CachedInteractionComponent = OwningPawn->FindComponentByClass<UFTInteractionComponent>();
	return CachedInteractionComponent;
}

UFTCaptureEscapeComponent* UFTInteractionStatusWidget::ResolveCaptureEscapeComponent()
{
	if (CachedCaptureEscapeComponent)
	{
		return CachedCaptureEscapeComponent;
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return nullptr;
	}

	CachedCaptureEscapeComponent = OwningPawn->FindComponentByClass<UFTCaptureEscapeComponent>();
	return CachedCaptureEscapeComponent;
}

void UFTInteractionStatusWidget::UpdatePrompt(UFTInteractionComponent* InteractionComponent)
{
	AActor* FocusedActor = InteractionComponent ? InteractionComponent->GetFocusedActor() : nullptr;
	if (!FocusedActor)
	{
		SetPromptText(FText::GetEmpty());
		return;
	}

	FText PromptText = FText::FromString(TEXT("상호작용"));
	if (FocusedActor->Implements<UFTInteractable>())
	{
		PromptText = IFTInteractable::Execute_GetInteractionPrompt(FocusedActor);
	}

	SetPromptText(PromptText);
}

void UFTInteractionStatusWidget::SetWidgetOptionalVisibility(UWidget* Widget, bool bVisible) const
{
	if (Widget)
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UFTInteractionStatusWidget::SetStatusMode(EFTInteractionStatusModeType NewMode)
{
	if (CurrentMode == NewMode)
	{
		return;
	}

	CurrentMode = NewMode;
	const bool bShowPrompt = CurrentMode == EFTInteractionStatusModeType::Prompt;
	const bool bShowProgress = CurrentMode == EFTInteractionStatusModeType::ShelfSearch || CurrentMode == EFTInteractionStatusModeType::Escape;

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(CurrentMode == EFTInteractionStatusModeType::None ? 0.0f : 1.0f);
	SetWidgetOptionalVisibility(WBP_Progress, bShowProgress);

	if (bShowPrompt)
	{
		SetWidgetOptionalVisibility(WBP_InteractionPrompt, true);
	}
	else if (WBP_InteractionPrompt)
	{
		WBP_InteractionPrompt->HidePrompt();
	}

	if (UWidgetSwitcher* InteractionSwitcher = GetInteractionSwitcher())
	{
		if (bShowPrompt && WBP_InteractionPrompt)
		{
			InteractionSwitcher->SetActiveWidget(WBP_InteractionPrompt);
		}
		else if (bShowProgress && WBP_Progress)
		{
			InteractionSwitcher->SetActiveWidget(WBP_Progress);
		}
	}
}

void UFTInteractionStatusWidget::SetPromptText(const FText& PromptText)
{
	if (PromptText.IsEmpty())
	{
		SetStatusMode(EFTInteractionStatusModeType::None);
		return;
	}

	if (WBP_InteractionPrompt)
	{
		WBP_InteractionPrompt->ShowPrompt(PromptText);
	}
}

UWidgetSwitcher* UFTInteractionStatusWidget::GetInteractionSwitcher() const
{
	return WS_InteractionState ? WS_InteractionState.Get() : WidgetSwitcher_47.Get();
}
