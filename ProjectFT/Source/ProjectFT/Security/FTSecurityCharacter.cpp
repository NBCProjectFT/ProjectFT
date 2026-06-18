
#include "FTSecurityCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"


AFTSecurityCharacter::AFTSecurityCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
}

void AFTSecurityCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AFTSecurityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AFTSecurityCharacter::SetMoveSpeed(float NewSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}
