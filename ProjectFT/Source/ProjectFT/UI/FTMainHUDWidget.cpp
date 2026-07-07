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
#include "FTUIManagerSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"

void UFTMainHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveHUDBarWidgets();
	ResolveHUDViewModel();
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
