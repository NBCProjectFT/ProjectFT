#include "FTHUDViewModel.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/AbilitySystem/FTPlayerAttributeSet.h"

UFTHUDViewModel::UFTHUDViewModel()
{	
}

UFTHUDViewModel::~UFTHUDViewModel()
{
	ClearPlayerBinding();
}

void UFTHUDViewModel::InitializeFromPlayer(APawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return;
	}

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(PlayerPawn);
	if (!AbilitySystemInterface)
	{
		return;
	}

	InitializeFromAbilitySystem(AbilitySystemInterface->GetAbilitySystemComponent());
}

void UFTHUDViewModel::ClearPlayerBinding()
{
	UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		BoundAbilitySystemComponent.Reset();
		HealthChangedHandle.Reset();
		MaxHealthChangedHandle.Reset();
		StaminaChangedHandle.Reset();
		MaxStaminaChangedHandle.Reset();
		return;
	}

	if (HealthChangedHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
	}

	if (MaxHealthChangedHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
	}

	if (StaminaChangedHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTPlayerAttributeSet::GetStaminaAttribute()).Remove(StaminaChangedHandle);
	}

	if (MaxStaminaChangedHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTPlayerAttributeSet::GetMaxStaminaAttribute()).Remove(MaxStaminaChangedHandle);
	}

	BoundAbilitySystemComponent.Reset();
	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	StaminaChangedHandle.Reset();
	MaxStaminaChangedHandle.Reset();
}

bool UFTHUDViewModel::IsPlayerBound() const
{
	return BoundAbilitySystemComponent.IsValid();
}

void UFTHUDViewModel::RefreshPlayerAttributes()
{
	RefreshAttributeValues();
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

void UFTHUDViewModel::SetCrosshairState(const FTCrosshairStateStruct& NewCrosshairState)
{
	CrosshairState = NewCrosshairState;
	NotifyChanged();
}

const FTCrosshairStateStruct& UFTHUDViewModel::GetCrosshairState() const
{
	return CrosshairState;
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

void UFTHUDViewModel::InitializeFromAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent || BoundAbilitySystemComponent.Get() == AbilitySystemComponent)
	{
		RefreshAttributeValues();
		return;
	}

	ClearPlayerBinding();

	BoundAbilitySystemComponent = AbilitySystemComponent;

	HealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ThisClass::OnHealthAttributeChanged);
	MaxHealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &ThisClass::OnMaxHealthAttributeChanged);
	StaminaChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTPlayerAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &ThisClass::OnStaminaAttributeChanged);
	MaxStaminaChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTPlayerAttributeSet::GetMaxStaminaAttribute())
		.AddUObject(this, &ThisClass::OnMaxStaminaAttributeChanged);

	RefreshAttributeValues();
}

void UFTHUDViewModel::RefreshAttributeValues()
{
	const UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		return;
	}

	SetHPValue(
		AbilitySystemComponent->GetNumericAttribute(UFTAttributeSet::GetHealthAttribute()),
		AbilitySystemComponent->GetNumericAttribute(UFTAttributeSet::GetMaxHealthAttribute()));

	SetStamina(
		AbilitySystemComponent->GetNumericAttribute(UFTPlayerAttributeSet::GetStaminaAttribute()),
		AbilitySystemComponent->GetNumericAttribute(UFTPlayerAttributeSet::GetMaxStaminaAttribute()));
}

void UFTHUDViewModel::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshAttributeValues();
}

void UFTHUDViewModel::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshAttributeValues();
}

void UFTHUDViewModel::OnStaminaAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshAttributeValues();
}

void UFTHUDViewModel::OnMaxStaminaAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshAttributeValues();
}
