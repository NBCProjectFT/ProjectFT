// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_UseItem.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGA_UseItem::UFTGA_UseItem()
{
	// 시전(활성) 중 소유자에게 상태 태그를 부여(ActivationOwnedTags) → "아이템 동작 진행 중?" 가드 질의에 쓰인다.
	ActivationOwnedTags.AddTag(TAG_FT_State_UsingItem);

	// 식별 AssetTag(시전형). CancelAbilities는 AssetTags를 매칭하므로 취소 대상은 이 태그로 잡힌다.
	// 이동 시(.Channeled) + 퀵슬롯 전환 시(부모 Ability.ItemUse) 모두 취소된다.
	{
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(TAG_FT_Ability_ItemUse_Channeled);
		SetAssetTags(AssetTags);
	}

	// 아이템 사용 입력이 보내는 GameplayEvent(Event.UseItem)로 발동된다(페이로드 = 대상 UFTItemDataAsset).
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_UseItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_UseItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 발동 페이로드의 아이템 데이터에서 사용 정보를 읽어 캐싱한다(베이스). 없으면 사용 불가로 취소.
	if (!CacheActiveItem(TriggerEventData))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 쿨다운 차단은 사용 입력 시 호출측(AFTPlayerCharacter)이 아이템별 태그로 이미 걸러낸다. 여기서는 시전 흐름만 진행한다.
	if (ActiveUseData.CastTimeSeconds > 0.0f)
	{
		// 시전시간 동안 대기 후 효과 적용. 시전 중에는 동일 어빌리티가 활성 상태라 재사용이 막힌다.
		UAbilityTask_WaitDelay* CastTask = UAbilityTask_WaitDelay::WaitDelay(this, ActiveUseData.CastTimeSeconds);
		CastTask->OnFinish.AddDynamic(this, &UFTGA_UseItem::OnCastFinished);
		CastTask->ReadyForActivation();
	}
	else
	{
		FinishUse();
	}
}

void UFTGA_UseItem::OnCastFinished()
{
	FinishUse();
}

void UFTGA_UseItem::FinishUse()
{
	// 비용/쿨다운을 표준 경로로 커밋(효과 적용 "전"에 → 비용 부족이면 효과 없이 취소).
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// UseEffects를 자신에게 적용(베이스 헬퍼, TargetData 없음 → Owner).
	ApplyUseEffects(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	
	OnItemConsumed();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}