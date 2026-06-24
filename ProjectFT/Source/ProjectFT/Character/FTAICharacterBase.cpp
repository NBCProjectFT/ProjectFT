#include "FTAICharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/Core/FTLogChannels.h"

AFTAICharacterBase::AFTAICharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFTAICharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AttributeSet)
	{
		AttributeSet->SetMaxHealth(InitialHealth);
		AttributeSet->SetHealth(InitialHealth);
		AttributeSet->SetMoveSpeed(InitialMoveSpeed);
		ApplyMovementSpeed();
	}
}

void AFTAICharacterBase::SetMoveSpeed(float NewSpeed)
{
	if (AttributeSet)
	{
		AttributeSet->SetMoveSpeed(NewSpeed);
		ApplyMovementSpeed();
	}
}

void AFTAICharacterBase::HandleDeath()
{
	UE_LOG(LogFTNPC, Log, TEXT("AI character '%s' died."), *GetNameSafe(this));

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->DisableMovement();
	}
}
