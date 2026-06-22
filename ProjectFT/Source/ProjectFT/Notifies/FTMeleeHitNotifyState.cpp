#include "FTMeleeHitNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectFT/Components/FTEquipmentComponent.h"
#include "ProjectFT/Message/FTGameplayTags.h"

namespace
{
	UFTEquipmentComponent* ResolveEquipmentComponent(USkeletalMeshComponent* MeshComp)
	{
		AActor* Character = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Character
			? Character->FindComponentByClass<UFTEquipmentComponent>()
			: nullptr;
	}
}

UFTMeleeHitNotifyState::UFTMeleeHitNotifyState()
{
	ActionTag = TAG_FT_Weapon_Action_Primary;
}

void UFTMeleeHitNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UFTEquipmentComponent* Equipment = ResolveEquipmentComponent(MeshComp))
	{
		Equipment->NotifyWeaponActionWindowBegin(ActionTag);
	}
}

void UFTMeleeHitNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (UFTEquipmentComponent* Equipment = ResolveEquipmentComponent(MeshComp))
	{
		Equipment->NotifyWeaponActionWindowTick(ActionTag);
	}
}

void UFTMeleeHitNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UFTEquipmentComponent* Equipment = ResolveEquipmentComponent(MeshComp))
	{
		Equipment->NotifyWeaponActionWindowEnd(ActionTag);
	}
}
