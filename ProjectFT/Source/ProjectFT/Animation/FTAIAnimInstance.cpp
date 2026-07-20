#include "FTAIAnimInstance.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Character/FTAICharacterBase.h"

void UFTAIAnimInstance::CacheOwnerReferences()
{
	Super::CacheOwnerReferences();

	AICharacter = Cast<AFTAICharacterBase>(OwnerCharacter);
}

void UFTAIAnimInstance::UpdateCharacterState(float DeltaSeconds)
{
	Super::UpdateCharacterState(DeltaSeconds);

	NormalizedGroundSpeed = MovementReferenceSpeed > KINDA_SMALL_NUMBER
		? GroundSpeed / MovementReferenceSpeed
		: 0.0f;
	StrafeDirection = GroundSpeed > KINDA_SMALL_NUMBER
		? StrafeSpeed / GroundSpeed
		: 0.0f;
	bShouldMove = GroundSpeed > 3.0f;
	bIsStunned = bIsImmobilized && ImmobilizedTag.MatchesTagExact(TAG_FT_State_Debuff_Stun);
}
