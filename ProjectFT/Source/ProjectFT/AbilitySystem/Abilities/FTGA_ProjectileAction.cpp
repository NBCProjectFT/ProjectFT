#include "FTGA_ProjectileAction.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTProjectileActorDataAsset.h"

UFTGA_ProjectileAction::UFTGA_ProjectileAction()
{
	// 투사체 적중 이벤트마다 어떤 ProjectileActorData를 썼는지 들고 있어야 한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// AFTProjectileActor가 충돌/폭발 시 Source ASC로 Event.TargetHit을 보내면 이 Ability가 실행된다.
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

	// 이 Ability는 입력으로 직접 실행되지 않고 ProjectileActor의 TargetHit 이벤트로만 실행된다.
	UE_LOG(LogTemp, Warning, TEXT("[ProjectileDebug] ProjectileAction ActivateAbility entered. ActorInfoAvatar=%s"),
		ActorInfo && ActorInfo->AvatarActor.IsValid() ? *GetNameSafe(ActorInfo->AvatarActor.Get()) : TEXT("None"));
	
	if (!TriggerEventData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ProjectileAction failed: TriggerEventData is null."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Payload.OptionalObject에는 충돌한 ProjectileActor가 사용한 ProjectileActorDataAsset이 들어온다.
	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);

	ActiveItemData = const_cast<UFTItemDataAsset*>(ItemAsset);
	ProjectileActorData = Cast<UFTProjectileActorDataAsset>(ActiveItemData);

	// ProjectileActorDataAsset의 UseData.UseEffects가 실제 적용할 효과 목록이다.
	if (!ActiveItemData || !ProjectileActorData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ProjectileAction failed: invalid ProjectileActorData. OptionalObject=%s"),
			*GetNameSafe(TriggerEventData->OptionalObject.Get()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Payload.Target이 실제 효과를 받을 액터다.
	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());

	if (!TargetActor)
	{
		UE_LOG(LogFTItem, Warning, TEXT("ProjectileAction failed: TargetActor is null."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ProjectileDebug] ProjectileAction payload ok. Item=%s ItemId=%s Target=%s UseEffects=%d Magnitudes=%d"),
		*GetNameSafe(ProjectileActorData),
		*ProjectileActorData->ItemData.ItemId.ToString(),
		*GetNameSafe(TargetActor),
		ActiveUseData.UseEffects.Num(),
		ActiveUseData.EffectMagnitudes.Num());

	for (const TSubclassOf<UGameplayEffect>& EffectClass : ActiveUseData.UseEffects)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectileDebug]   Effect=%s"), *GetNameSafe(EffectClass.Get()));
	}

	for (const TPair<FGameplayTag, float>& Magnitude : ActiveUseData.EffectMagnitudes)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProjectileDebug]   Magnitude Tag=%s Value=%.3f"),
			*Magnitude.Key.ToString(),
			Magnitude.Value);
	}

	// TargetActor를 GAS TargetData로 바꿔 베이스의 ApplyUseEffects 경로를 그대로 사용한다.
	FGameplayAbilityTargetDataHandle TargetDataHandle =
		UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(TargetActor);

	// 데미지/스턴/상태이상 등 실제 효과는 ProjectileActorDataAsset의 UseEffects가 처리한다.
	ApplyUseEffects(
		Handle,
		ActorInfo,
		ActivationInfo,
		&TargetDataHandle
	);

	UE_LOG(LogTemp, Warning, TEXT("[ProjectileDebug] ProjectileAction ApplyUseEffects finished. Target=%s"), *GetNameSafe(TargetActor));
	
	// 적중 처리 한 번으로 끝나는 Ability라 캐시를 비우고 정상 종료한다.
	ActiveItemData = nullptr;
	ProjectileActorData = nullptr;

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
