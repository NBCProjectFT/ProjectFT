#include "FTPlayerAnimInstance.h"

#include "ProjectFT/Player/FTPlayerCharacter.h"

void UFTPlayerAnimInstance::CacheOwnerReferences()
{
	Super::CacheOwnerReferences();

	PlayerCharacter = Cast<AFTPlayerCharacter>(OwnerCharacter);
}

void UFTPlayerAnimInstance::UpdateCharacterState(float DeltaSeconds)
{
	Super::UpdateCharacterState(DeltaSeconds);

	if (!PlayerCharacter)
	{
		bIsCaptured = false;
		bIsChannelingInteraction = false;
		bIsInventoryOpen = false;
		HeldWeaponStance = EFTWeaponStanceType::Unarmed;
		SprintMovementSpeed = 0.0f;
		CrouchMovementSpeed = 0.0f;
		NormalizedGroundSpeed = 0.0f;
		LocomotionPlayRate = 1.0f;
		CrouchPlayRate = 1.0f;
		return;
	}

	bIsCaptured = PlayerCharacter->IsCaptured();
	bIsChannelingInteraction = PlayerCharacter->IsChannelingInteraction();
	bIsInventoryOpen = PlayerCharacter->IsInventoryOpen();
	HeldWeaponStance = PlayerCharacter->GetHeldWeaponStance();
	SprintMovementSpeed = PlayerCharacter->GetSprintMovementSpeed();
	CrouchMovementSpeed = PlayerCharacter->GetCrouchMovementSpeed();

	NormalizedGroundSpeed = SprintMovementSpeed > KINDA_SMALL_NUMBER
		? GroundSpeed / SprintMovementSpeed
		: 0.0f;
	if (SprintMovementSpeed > KINDA_SMALL_NUMBER)
	{
		ForwardSpeed /= SprintMovementSpeed;
		StrafeSpeed /= SprintMovementSpeed;
	}
	else
	{
		ForwardSpeed = 0.0f;
		StrafeSpeed = 0.0f;
	}

	LocomotionPlayRate = RunningReferenceSpeed > KINDA_SMALL_NUMBER
		? SprintMovementSpeed / RunningReferenceSpeed
		: 1.0f;
	CrouchPlayRate = CrouchReferenceSpeed > KINDA_SMALL_NUMBER
		? CrouchMovementSpeed / CrouchReferenceSpeed
		: 1.0f;
}
