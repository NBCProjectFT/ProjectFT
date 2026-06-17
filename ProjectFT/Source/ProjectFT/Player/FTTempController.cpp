#include "FTTempController.h"
#include "EnhancedInputSubsystems.h"

AFTTempController::AFTTempController()
{
	PrimaryActorTick.bCanEverTick = true;
}

AFTTempController::~AFTTempController()
{
	
}

void AFTTempController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
}
