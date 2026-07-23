#include "FTGrabCaptureWindowNotifyState.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#include "ProjectFT/AbilitySystem/Abilities/FTGA_Grab.h"

void UFTGrabCaptureWindowNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	ForEachActiveGrabAbility(MeshComp, [](UFTGA_Grab& GrabAbility)
	{
		GrabAbility.OpenGrabCaptureWindow();
	});
}

void UFTGrabCaptureWindowNotifyState::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	ForEachActiveGrabAbility(MeshComp, [](UFTGA_Grab& GrabAbility)
	{
		GrabAbility.CloseGrabCaptureWindow();
	});
}

void UFTGrabCaptureWindowNotifyState::ForEachActiveGrabAbility(
	const USkeletalMeshComponent* MeshComp,
	TFunctionRef<void(UFTGA_Grab&)> Callback) const
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	const IAbilitySystemInterface* AbilitySystemActor = Cast<IAbilitySystemInterface>(OwnerActor);
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemActor
		? AbilitySystemActor->GetAbilitySystemComponent()
		: nullptr;

	if (!AbilitySystemComponent)
	{
		return;
	}

	FScopedAbilityListLock AbilityListLock(*AbilitySystemComponent);
	for (FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			UFTGA_Grab* GrabAbility = Cast<UFTGA_Grab>(AbilityInstance);
			if (GrabAbility)
			{
				Callback(*GrabAbility);
			}
		}
	}
}
