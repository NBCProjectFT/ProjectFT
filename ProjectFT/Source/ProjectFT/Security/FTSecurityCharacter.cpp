
#include "FTSecurityCharacter.h"


AFTSecurityCharacter::AFTSecurityCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFTSecurityCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AFTSecurityCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFTSecurityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

