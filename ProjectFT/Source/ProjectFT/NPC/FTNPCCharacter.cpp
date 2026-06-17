#include "FTNPCCharacter.h"



AFTNPCCharacter::AFTNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFTNPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AFTNPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFTNPCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

