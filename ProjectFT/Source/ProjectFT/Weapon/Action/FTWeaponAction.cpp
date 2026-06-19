#include "FTWeaponAction.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
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

	if (!PlayAttackMontage())
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

bool UFTWeaponAction::PlayAttackMontage() const
{
	if (Definition.AttackMontage.IsNull())
	{
		return true;
	}

	AActor* WeaponActor = ActionComponent ? ActionComponent->GetOwner() : nullptr;
	ACharacter* Character = WeaponActor ? Cast<ACharacter>(WeaponActor->GetOwner()) : nullptr;
	UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	UAnimMontage* Montage = Definition.AttackMontage.LoadSynchronous();

	return AnimInstance && Montage && AnimInstance->Montage_Play(Montage) > 0.0f;
}

UWorld* UFTWeaponAction::GetWorld() const
{
	return ActionComponent ? ActionComponent->GetWorld() : nullptr;
}
