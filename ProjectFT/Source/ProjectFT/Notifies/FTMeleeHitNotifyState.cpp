#include "FTMeleeHitNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectFT/Components/FTQuickSlotComponent.h"
#include "ProjectFT/Message/FTGameplayTags.h"

namespace
{
	UFTQuickSlotComponent* ResolveQuickSlotComponent(USkeletalMeshComponent* MeshComp)
	{
		AActor* Character = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Character
			? Character->FindComponentByClass<UFTQuickSlotComponent>()
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
	if (UFTQuickSlotComponent* QuickSlot = ResolveQuickSlotComponent(MeshComp))
	{
		QuickSlot->NotifyWeaponActionWindowBegin(ActionTag);
	}
}

void UFTMeleeHitNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (UFTQuickSlotComponent* QuickSlot = ResolveQuickSlotComponent(MeshComp))
	{
		QuickSlot->NotifyWeaponActionWindowTick(ActionTag);
	}
}

void UFTMeleeHitNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UFTQuickSlotComponent* QuickSlot = ResolveQuickSlotComponent(MeshComp))
	{
		QuickSlot->NotifyWeaponActionWindowEnd(ActionTag);
	}
}
