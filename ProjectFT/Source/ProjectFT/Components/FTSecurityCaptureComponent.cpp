#include "FTSecurityCaptureComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Controller.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTSecurityCaptureComponent::UFTSecurityCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFTSecurityCaptureComponent::TryCaptureTarget(
	AActor* SecurityActor,
	AActor* TargetActor,
	FVector CaptureLocation)
{
	(void)CaptureLocation;

	if (const AController* SecurityController = Cast<AController>(SecurityActor))
	{
		SecurityActor = SecurityController->GetPawn();
	}

	IAbilitySystemInterface* AbilitySystemActor = Cast<IAbilitySystemInterface>(SecurityActor);
	UAbilitySystemComponent* ASC = AbilitySystemActor ? AbilitySystemActor->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !IsValid(TargetActor))
	{
		return false;
	}

	FGameplayEventData Payload;
	Payload.EventTag = TAG_FT_Event_Grab;
	Payload.Instigator = SecurityActor;
	Payload.Target = TargetActor;
	return ASC->HandleGameplayEvent(TAG_FT_Event_Grab, &Payload) > 0;
}
