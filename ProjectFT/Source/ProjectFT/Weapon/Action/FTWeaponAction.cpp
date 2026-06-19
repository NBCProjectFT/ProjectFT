#include "FTWeaponAction.h"

#include "ProjectFT/Components/FTWeaponActionComponent.h"

void UFTWeaponAction::Initialize(UFTWeaponActionComponent* InActionComponent,
	const FFTWeaponActionDefinition& InDefinition)
{
	ActionComponent = InActionComponent;
	Definition = InDefinition;
}

bool UFTWeaponAction::StartAction()
{
	UWorld* World = GetWorld();
	if (!ActionComponent || !World)
	{
		return false;
	}

	const double CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastExecutionTime < Definition.Cooldown)
	{
		return false;
	}

	if (!ExecuteAction())
	{
		return false;
	}

	LastExecutionTime = CurrentTime;
	return true;
}

UWorld* UFTWeaponAction::GetWorld() const
{
	return ActionComponent ? ActionComponent->GetWorld() : nullptr;
}
