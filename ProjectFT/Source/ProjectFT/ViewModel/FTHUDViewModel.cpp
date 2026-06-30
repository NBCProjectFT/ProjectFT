#include "FTHUDViewModel.h"

UFTHUDViewModel::UFTHUDViewModel()
{	
}

UFTHUDViewModel::~UFTHUDViewModel()
{
}

void UFTHUDViewModel::NotifyChanged()
{
}

void UFTHUDViewModel::SetHPPercent(float NewHPPercent)
{
	TargetHPPercent = NormalizePercent(NewHPPercent);
	HP = TargetHPPercent * 100.0f;
	NotifyChanged();
}

void UFTHUDViewModel::SetHPValue(float CurrentHP, float MaxHP)
{
	HP = FMath::Max(CurrentHP, 0.0f);

	if (MaxHP <= 0.0f)
	{
		TargetHPPercent = 0.0f;
		NotifyChanged();
		return;
	}

	TargetHPPercent = FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);
	NotifyChanged();
}

void UFTHUDViewModel::SetStamina(float CurrentStamina, float MaxStamina)
{
	Stamina = FMath::Max(CurrentStamina, 0.0f) ;
	
	if (MaxStamina <= 0.0f)
	{
		TargetStaminaPercent = 0.0f;
		NotifyChanged();
		return;
	}
	
	TargetStaminaPercent = FMath::Clamp(CurrentStamina / MaxStamina, 0.0f, 1.0f);
	NotifyChanged();
}

void UFTHUDViewModel::SetWeight(float NewCurrentWeight, float NewMaxWeight)
{
	CurrentWeight = NewCurrentWeight;
	MaxWeight = NewMaxWeight;
	NotifyChanged();
}

void UFTHUDViewModel::SetReportGauge(float NewReportGauge)
{
	ReportGauge = NewReportGauge;
	NotifyChanged();
}

void UFTHUDViewModel::SetObjectiveText(const FText& NewObjectiveText)
{
	ObjectiveText = NewObjectiveText;
	NotifyChanged();
}

float UFTHUDViewModel::GetTargetHPPercent() const
{
	return TargetHPPercent;
}

float UFTHUDViewModel::GetTargetStaminaPercent() const
{
	return TargetStaminaPercent;
}

void UFTHUDViewModel::TestCode()
{
	
}

UFTItemSlotDataObject* UFTHUDViewModel::GetOrCreateItemSlot(int32 SlotIndex)
{
	if (SlotIndex < 0)
	{
		return nullptr;
	}

	for (UFTItemSlotDataObject* ItemSlot : ItemSlotObjects)
	{
		if (ItemSlot && ItemSlot->SlotIndex == SlotIndex)
		{
			return ItemSlot;
		}
	}

	UFTItemSlotDataObject* NewItemSlot = NewObject<UFTItemSlotDataObject>(this);
	if (NewItemSlot)
	{
		NewItemSlot->SlotIndex = SlotIndex;
		ItemSlotObjects.Add(NewItemSlot);
	}

	return NewItemSlot;
}

float UFTHUDViewModel::NormalizePercent(float Value) const
{
	if (Value > 1.0f)
	{
		return FMath::Clamp(Value / 100.0f, 0.0f, 1.0f);
	}

	return FMath::Clamp(Value, 0.0f, 1.0f);
}
