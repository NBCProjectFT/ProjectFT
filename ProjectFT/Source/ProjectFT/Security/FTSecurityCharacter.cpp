
#include "FTSecurityCharacter.h"


AFTSecurityCharacter::AFTSecurityCharacter()
{
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
	Super::SetMoveSpeed(NewSpeed);
}
