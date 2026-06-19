#include "FTMeleeHitNotifyState.h"

#include "Components/SkeletalMeshComponent.h"
#include "ProjectFT/Components/FTEquipmentComponent.h"
#include "ProjectFT/Components/FTWeaponActionComponent.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Weapon/FTWeaponActor.h"

namespace
{
	UFTWeaponActionComponent* ResolveWeaponActionComponent(USkeletalMeshComponent* MeshComp)
	{
		AActor* Character = MeshComp ? MeshComp->GetOwner() : nullptr;
		UFTEquipmentComponent* Equipment = Character
			? Character->FindComponentByClass<UFTEquipmentComponent>()
			: nullptr;
		AFTWeaponActor* Weapon = Equipment ? Equipment->GetEquippedWeapon() : nullptr;
		return Weapon ? Weapon->GetActionComponent() : nullptr;
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
	if (UFTWeaponActionComponent* ActionComponent = ResolveWeaponActionComponent(MeshComp))
	{
		ActionComponent->NotifyActionWindowBegin(ActionTag);
	}
}

void UFTMeleeHitNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (UFTWeaponActionComponent* ActionComponent = ResolveWeaponActionComponent(MeshComp))
	{
		ActionComponent->NotifyActionWindowTick(ActionTag);
	}
}

void UFTMeleeHitNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UFTWeaponActionComponent* ActionComponent = ResolveWeaponActionComponent(MeshComp))
	{
		ActionComponent->NotifyActionWindowEnd(ActionTag);
	}
}
