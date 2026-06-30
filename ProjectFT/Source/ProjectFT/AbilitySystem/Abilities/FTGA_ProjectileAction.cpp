#include "FTGA_ProjectileAction.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTProjectileActorDataAsset.h"

UFTGA_ProjectileAction::UFTGA_ProjectileAction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_TargetHit;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_ProjectileAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("ProjectileAction Activated"));
	
	if (!TriggerEventData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ProjectileAction failed: TriggerEventData is null."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);

	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ProjectileActorData = Cast<UFTProjectileActorDataAsset>(ActiveItemData);

	if (!ActiveItemData || !ProjectileActorData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ProjectileAction failed: invalid ProjectileActorData."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());

	if (!TargetActor)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ProjectileAction failed: TargetActor is null."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle =
		UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);

	ApplyUseEffects(
		Handle,
		ActorInfo,
		ActivationInfo,
		&TargetDataHandle
	);
	
	ActiveItemData = nullptr;
	ProjectileActorData = nullptr;

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}