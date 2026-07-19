#include "FTMainHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "FTItemSlotEntryWidget.h"
#include "FTItemSlotListView.h"
#include "FTInteractionPromptWidget.h"
#include "FTUIManagerSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"

void UFTMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveHUDBarWidgets();
	ResolveHUDViewModel();
	CreateInteractionPromptWidget();
	ResolveInteractionPromptBinding();
	if (HUDViewModel)
	{
		HUDViewModel->InitializeFromPlayer(GetOwningPlayerPawn());
	}

	const float InitialHPPercent = HUDViewModel ? HUDViewModel->GetTargetHPPercent() : 1.0f;
	CurrentHPFrontPercent = InitialHPPercent;
	CurrentHPBackPercent = InitialHPPercent;
	const float InitialStaminaPercent = HUDViewModel ? HUDViewModel->GetTargetStaminaPercent() : 1.0f;
	CurrentStaminaFrontPercent = InitialStaminaPercent;
	CurrentStaminaBackPercent = InitialStaminaPercent;
	UpdateHPBars(0.0f);
	UpdateStaminaBar(0.0f);
}

void UFTMainHUDWidget::NativeDestruct()
{
	ClearInteractionPromptBinding();
	RemoveInteractionPromptWidget();

	Super::NativeDestruct();
}

void UFTMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (HUDViewModel)
	{
		if (!HUDViewModel->IsPlayerBound())
		{
			HUDViewModel->InitializeFromPlayer(GetOwningPlayerPawn());
		}
		HUDViewModel->RefreshPlayerAttributes();
	}

	UpdateHPBars(InDeltaTime);
	UpdateStaminaBar(InDeltaTime);
	UpdateCrosshair();
	ResolveInteractionPromptBinding();
	
	// UFTAssetManager::GetAsset();
}

void UFTMainHUDWidget::UpdateHP(float NewHP)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetHPPercent(NewHP);
	}
}

void UFTMainHUDWidget::UpdateHPValue(float CurrentHP, float MaxHP)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetHPValue(CurrentHP, MaxHP);
	}
}

void UFTMainHUDWidget::UpdateStamina(float CurrentStamina, float MaxStemina)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetStamina(CurrentStamina, MaxStemina);
	}
}

void UFTMainHUDWidget::UpdateWeight(float CurrentWeight, float MaxWeight)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetWeight(CurrentWeight, MaxWeight);
	}
}

void UFTMainHUDWidget::UpdateReportGauge(float NewReportGauge)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetReportGauge(NewReportGauge);
	}
}

void UFTMainHUDWidget::UpdateObjective(const FText& NewObjectiveText)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetObjectiveText(NewObjectiveText);
	}
}

int32 UFTMainHUDWidget::GetQuickSlotCount() const
{
	return HB_QuickSlot ? HB_QuickSlot->GetChildrenCount() : 0;
}

UWidget* UFTMainHUDWidget::GetQuickSlotWidget(int32 SlotIndex) const
{
	if (!HB_QuickSlot || SlotIndex < 0 || SlotIndex >= HB_QuickSlot->GetChildrenCount())
	{
		return nullptr;
	}

	return HB_QuickSlot->GetChildAt(SlotIndex);
}

TArray<UWidget*> UFTMainHUDWidget::GetQuickSlotWidgets() const
{
	TArray<UWidget*> QuickSlotWidgets;

	if (!HB_QuickSlot)
	{
		return QuickSlotWidgets;
	}

	const int32 SlotCount = HB_QuickSlot->GetChildrenCount();
	QuickSlotWidgets.Reserve(SlotCount);

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		QuickSlotWidgets.Add(HB_QuickSlot->GetChildAt(SlotIndex));
	}

	return QuickSlotWidgets;
}

void UFTMainHUDWidget::HandleFocusedInteractableChanged(AActor* FocusedActor)
{
	if (!InteractionPromptWidget)
	{
		return;
	}

	if (!FocusedActor)
	{
		InteractionPromptWidget->HidePrompt();
		return;
	}

	FText PromptText = FText::FromString(TEXT("상호작용"));
	if (FocusedActor->Implements<UFTInteractable>())
	{
		PromptText = IFTInteractable::Execute_GetInteractionPrompt(FocusedActor);
	}

	if (PromptText.IsEmpty())
	{
		InteractionPromptWidget->HidePrompt();
		return;
	}

	InteractionPromptWidget->ShowPrompt(PromptText);
}

void UFTMainHUDWidget::UpdateHPBars(float DeltaTime)
{
	const float TargetHPPercent = HUDViewModel ? HUDViewModel->GetTargetHPPercent() : 1.0f;

	CurrentHPFrontPercent = FMath::FInterpTo(CurrentHPFrontPercent, TargetHPPercent, DeltaTime, HPFrontInterpSpeed);
	CurrentHPBackPercent = FMath::FInterpTo(CurrentHPBackPercent, TargetHPPercent, DeltaTime, HPBackInterpSpeed);

	if (MID_HPBar)
	{
		MID_HPBar->SetScalarParameterValue(TEXT("Value"), CurrentHPFrontPercent);
		MID_HPBar->SetScalarParameterValue(TEXT("TickDownValue"), CurrentHPBackPercent);
	}
}

void UFTMainHUDWidget::UpdateStaminaBar(float DeltaTime)
{
	const float TargetStaminaPercent = HUDViewModel ? HUDViewModel->GetTargetStaminaPercent() : 1.0f;

	CurrentStaminaFrontPercent = FMath::FInterpTo(CurrentStaminaFrontPercent, TargetStaminaPercent, DeltaTime, StaminaFrontInterpSpeed);
	CurrentStaminaBackPercent = FMath::FInterpTo(CurrentStaminaBackPercent, TargetStaminaPercent, DeltaTime, StaminaBackInterpSpeed);

	if (MID_StaminaBar)
	{
		MID_StaminaBar->SetScalarParameterValue(TEXT("Value"), CurrentStaminaFrontPercent);
		MID_StaminaBar->SetScalarParameterValue(TEXT("TickDownValue"), CurrentStaminaBackPercent);
	}
}

void UFTMainHUDWidget::UpdateCrosshair()
{
	if (!HUDViewModel)
	{
		return;
	}

	const FTCrosshairStateStruct& CrosshairState = HUDViewModel->GetCrosshairState();
	const ESlateVisibility CrosshairVisibility = CrosshairState.bVisible
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;

	const auto ApplyImage = [CrosshairVisibility, &CrosshairState](UImage* Image, const TSoftObjectPtr<UTexture2D>& Texture, const FVector2D& Translation)
	{
		if (!Image)
		{
			return;
		}

		Image->SetVisibility(CrosshairVisibility);
		Image->SetRenderTranslation(Translation);
		Image->SetColorAndOpacity(CrosshairState.Color);

		if (CrosshairVisibility == ESlateVisibility::Collapsed)
		{
			return;
		}

		if (UTexture2D* LoadedTexture = Texture.LoadSynchronous())
		{
			Image->SetBrushFromTexture(LoadedTexture, true);
		}
	};

	const float SpreadScaled = CrosshairState.SpreadMax * CrosshairState.Spread;

	ApplyImage(IMG_CrosshairCenter, CrosshairState.CenterTexture, FVector2D::ZeroVector);
	ApplyImage(IMG_CrosshairLeft, CrosshairState.LeftTexture, FVector2D(-SpreadScaled, 0.0f));
	ApplyImage(IMG_CrosshairRight, CrosshairState.RightTexture, FVector2D(SpreadScaled, 0.0f));
	ApplyImage(IMG_CrosshairTop, CrosshairState.TopTexture, FVector2D(0.0f, -SpreadScaled));
	ApplyImage(IMG_CrosshairBottom, CrosshairState.BottomTexture, FVector2D(0.0f, SpreadScaled));
}

void UFTMainHUDWidget::ResolveHUDBarWidgets()
{
	if (IMG_HPBar && MID_HPBar && IMG_StaminaBar && MID_StaminaBar)
	{
		return;
	}
	
	if (IMG_HPBar && !MID_HPBar)
	{
		MID_HPBar = IMG_HPBar->GetDynamicMaterial();
		if (MID_HPBar)
		{
			MID_HPBar->SetScalarParameterValue(TEXT("Value"), CurrentHPFrontPercent);
			MID_HPBar->SetScalarParameterValue(TEXT("TickDownValue"), CurrentHPBackPercent);
		}
	}

	if (IMG_StaminaBar && !MID_StaminaBar)
	{
		MID_StaminaBar = IMG_StaminaBar->GetDynamicMaterial();
		if (MID_StaminaBar)
		{
			MID_StaminaBar->SetScalarParameterValue(TEXT("Value"), CurrentStaminaFrontPercent);
			MID_StaminaBar->SetScalarParameterValue(TEXT("TickDownValue"), CurrentStaminaBackPercent);
		}
	}
}

void UFTMainHUDWidget::ResolveHUDViewModel()
{
	if (HUDViewModel)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManagerSubsystem = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			HUDViewModel = UIManagerSubsystem->HUDViewModel;
		}
	}

	if (!HUDViewModel)
	{
		HUDViewModel = NewObject<UFTHUDViewModel>(this);
	}
}

void UFTMainHUDWidget::ResolveInteractionPromptBinding()
{
	if (!InteractionPromptWidget)
	{
		CreateInteractionPromptWidget();
	}

	if (InteractionComponent)
	{
		return;
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return;
	}

	InteractionComponent = OwningPawn->FindComponentByClass<UFTInteractionComponent>();
	if (!InteractionComponent)
	{
		return;
	}

	InteractionComponent->OnFocusedInteractableChanged.AddUniqueDynamic(this, &ThisClass::HandleFocusedInteractableChanged);
	HandleFocusedInteractableChanged(InteractionComponent->GetFocusedActor());
}

void UFTMainHUDWidget::ClearInteractionPromptBinding()
{
	if (InteractionComponent)
	{
		InteractionComponent->OnFocusedInteractableChanged.RemoveDynamic(this, &ThisClass::HandleFocusedInteractableChanged);
		InteractionComponent = nullptr;
	}
}

void UFTMainHUDWidget::CreateInteractionPromptWidget()
{
	if (InteractionPromptWidget || !InteractionPromptWidgetClass)
	{
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return;
	}

	InteractionPromptWidget = CreateWidget<UFTInteractionPromptWidget>(OwningPlayer, InteractionPromptWidgetClass);
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->AddToViewport(15);
		InteractionPromptWidget->HidePrompt();
	}
}

void UFTMainHUDWidget::RemoveInteractionPromptWidget()
{
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->RemoveFromParent();
		InteractionPromptWidget = nullptr;
	}
}
