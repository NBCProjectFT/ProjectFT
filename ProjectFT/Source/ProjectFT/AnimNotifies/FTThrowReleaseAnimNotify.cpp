#include "FTThrowReleaseAnimNotify.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

void UFTThrowReleaseAnimNotify::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = TAG_FT_Event_ThrowRelease;
	EventData.Instigator = OwnerActor;
	EventData.Target = OwnerActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		TAG_FT_Event_ThrowRelease,
		EventData
	);
}