// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_ItemAbility.h"

#include "GameplayEffect.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "ProjectFT/AbilitySystem/Effects/FTGE_Cooldown.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/FTUseDataEffectLibrary.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

UFTGA_ItemAbility::UFTGA_ItemAbility()
{
	// 쿨다운은 공용 쿨다운 GE로 처리하되, 표준 CooldownGameplayEffectClass 경로가 아니라 어빌리티가 직접 적용한다.
	// (지속시간 = ActiveUseData.CooldownSeconds SetByCaller, 아이템별 태그 = DynamicGrantedTags, 차단 = 호출측 태그 판정.)
	// 표준 경로를 쓰면 GE의 정적 부여 태그로만 차단해 모든 아이템이 한 쿨다운을 공유하므로 독립 쿨다운이 불가능하다.
	CooldownEffectClass = UFTGE_Cooldown::StaticClass();

	// 모든 아이템 사용 동작의 공통 식별(Asset) 태그. CancelAbilities는 AssetTags를 매칭하므로 이 태그가 없는 어빌리티는
	// 어떤 취소 질의에도 잡히지 않는다 — 근접/히트스캔/런처가 자기 태그를 안 달아 취소 불가였던 원인이라 베이스에서 보장한다.
	// 자식이 SetAssetTags로 더 구체적인 태그(.Channeled/.Aimed)를 달면 이 값을 '대체'하지만, 그 태그들도 이 태그의 자식이라
	// 부모 질의(Ability.ItemUse)에는 그대로 매칭된다(HasAny는 보유 측 태그를 부모로 확장해 비교).
	{
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(TAG_FT_Ability_ItemUse);
		SetAssetTags(AssetTags);
	}
}

bool UFTGA_ItemAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	const bool bCommitted = Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);

	// 커밋 실패(비용 부족 등)면 사용 자체가 성립하지 않았으므로 소리도 내지 않는다.
	if (bCommitted)
	{
		PlayUseSound();
	}

	return bCommitted;
}

void UFTGA_ItemAbility::PlayUseSound() const
{
	if (!ActiveUseData.UseSound)
	{
		return;
	}

	// 사용자(아바타)에 붙여 재생 — 걸어가며 쓰거나 던지는 중에도 소리가 몸을 따라간다(피격음/발소리와 같은 방식).
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		UGameplayStatics::SpawnSoundAttached(ActiveUseData.UseSound, Avatar->GetRootComponent());
	}
}

const UFTItemDataAsset* UFTGA_ItemAbility::CacheActiveItem(const FGameplayEventData* TriggerEventData)
{
	const UFTItemDataAsset* ItemAsset = TriggerEventData ? Cast<UFTItemDataAsset>(TriggerEventData->OptionalObject) : nullptr;
	
	if (ItemAsset)
	{
		ActiveUseData = ItemAsset->ItemData.UseData;
		ActiveItemId = ItemAsset->ItemData.ItemId;
	}
	return ItemAsset;
}

void UFTGA_ItemAbility::ApplyUseEffects(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayAbilityTargetDataHandle* TargetData)
{
	UFTUseDataEffectLibrary::ApplyUseEffectsFromAbility(
		this,
		Handle,
		ActorInfo,
		ActivationInfo,
		ActiveUseData,
		TargetData);
}

void UFTGA_ItemAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (ActiveUseData.CooldownSeconds <= 0.0f || !CooldownEffectClass)
	{
		return; // 쿨다운 없음.
	}

	// 공용 쿨다운 GE에 이 아이템의 쿨다운 시간을 SetByCaller로 주입하고, 아이템별 태그를 동적으로 부여해 직접 적용한다.
	// (표준 CooldownGameplayEffectClass 경로 대신 직접 Apply: 어빌리티 단위 공유 차단이 아닌 아이템별 독립 쿨다운을 위해.)
	const FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownEffectClass, GetAbilityLevel(Handle, ActorInfo));
	if (CooldownSpec.IsValid())
	{
		// 아이템별 쿨다운 태그를 동적으로 부여(없으면 공용 폴백 Cooldown.ItemUse) → 호출측이 이 태그로 재사용을 차단한다.
		CooldownSpec.Data->DynamicGrantedTags.AddTag(ResolveCooldownTag(ActiveUseData));
		CooldownSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Cooldown, ActiveUseData.CooldownSeconds);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
	}
}

void UFTGA_ItemAbility::OnItemConsumed()
{
	// 아이템이 소비됐음을 메시지로 알린다(인벤토리 차감/퀘스트/통계 등 관심 시스템이 각자 구독해 반응).
	// 인벤토리를 직접 차감하지 않는 이유: 소비에 반응하는 리스너가 여럿이고, 획득(Event.Item.PickedUp)과 대칭을 이루기 위함.
	// 차감은 아바타를 Instigator로 받은 그 폰의 UFTInventoryComponent가 처리한다(본인 것만 거름).
	if (ActiveItemId.IsNone())
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = AvatarActor;
	Payload.ItemId = ActiveItemId;

	UGameplayMessageSubsystem::Get(AvatarActor).BroadcastMessage(TAG_FT_Event_ItemConsumed, Payload);
}

FGameplayTag UFTGA_ItemAbility::ResolveCooldownTag(const FTItemUseStruct& UseData)
{
	return UFTUseDataEffectLibrary::ResolveCooldownTag(UseData);
}
