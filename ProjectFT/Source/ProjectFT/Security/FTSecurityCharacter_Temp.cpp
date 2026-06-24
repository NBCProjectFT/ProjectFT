#include "FTSecurityCharacter_Temp.h"
#include "GameFramework/CharacterMovementComponent.h"


AFTSecurityCharacter_Temp::AFTSecurityCharacter_Temp()
{
	PrimaryActorTick.bCanEverTick = false;
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
}

void AFTSecurityCharacter_Temp::BeginPlay()
{
	Super::BeginPlay();

}

void AFTSecurityCharacter_Temp::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AFTSecurityCharacter_Temp::SetMoveSpeed(float NewSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}
