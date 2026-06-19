#include "FTWeaponActionComponent.h"

#include "ProjectFT/Data/FTWeaponDataAsset.h"
#include "ProjectFT/Weapon/Action/FTWeaponAction.h"

UFTWeaponActionComponent::UFTWeaponActionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFTWeaponActionComponent::InitializeActions(const UFTWeaponDataAsset* WeaponDataAsset)
{
	Actions.Empty();
	if (!WeaponDataAsset)
	{
		return false;
	}

	for (const FFTWeaponActionDefinition& Definition : WeaponDataAsset->Actions)
	{
		AddAction(Definition);
	}

	return !Actions.IsEmpty();
}

bool UFTWeaponActionComponent::StartAction(FGameplayTag ActionTag)
{
	if (const TObjectPtr<UFTWeaponAction>* Action = Actions.Find(ActionTag))
	{
		return (*Action)->StartAction();
	}

	return false;
}

void UFTWeaponActionComponent::NotifyActionWindowBegin(FGameplayTag ActionTag)
{
	if (const TObjectPtr<UFTWeaponAction>* Action = Actions.Find(ActionTag))
	{
		(*Action)->NotifyWindowBegin();
	}
}

void UFTWeaponActionComponent::NotifyActionWindowTick(FGameplayTag ActionTag)
{
	if (const TObjectPtr<UFTWeaponAction>* Action = Actions.Find(ActionTag))
	{
		(*Action)->NotifyWindowTick();
	}
}

void UFTWeaponActionComponent::NotifyActionWindowEnd(FGameplayTag ActionTag)
{
	if (const TObjectPtr<UFTWeaponAction>* Action = Actions.Find(ActionTag))
	{
		(*Action)->NotifyWindowEnd();
	}
}

bool UFTWeaponActionComponent::AddAction(const FFTWeaponActionDefinition& Definition)
{
	if (!Definition.ActionTag.IsValid() || !Definition.ActionClass || Actions.Contains(Definition.ActionTag))
	{
		return false;
	}

	TObjectPtr<UFTWeaponAction> Action = NewObject<UFTWeaponAction>(this, Definition.ActionClass.Get());
	Action->Initialize(this, Definition);
	Actions.Add(Definition.ActionTag, Action);
	return true;
}
