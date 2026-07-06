// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_EscapableDebuff.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGA_EscapableDebuff::UFTGA_EscapableDebuff()
{
	// State.Escapable이 붙는 순간 자동 발동, 사라지면(타이머 만료/외부 제거) 자동 종료.
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_State_Escapable;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	AbilityTriggers.Add(Trigger);

	// 이 어빌리티는 '행동불능 중'에 돌아야 하므로(비눗방울/빙결이 곧 Immobilized) 우산 차단에서 예외.
	ActivationBlockedTags.RemoveTag(TAG_FT_State_Debuff_Immobilized);

	// 잡기(두-바디)는 캡처 컴포넌트가 자체 게이지로 처리하므로, 잡힌 중에는 이 어빌리티가 활성화되지 않게 막는다.
	ActivationBlockedTags.AddTag(TAG_FT_State_Captured);
}

void UFTGA_EscapableDebuff::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공용 게이지 시작: 임계값 주입 + 리셋. 자가 상태이상은 저지력 없음(순수 연타).
	Gauge.Begin(EscapeThreshold, /*DecayPerSecond=*/0.0f);

	// 발버둥 입력(Event.Struggle)을 계속 수신(OnlyTriggerOnce=false).
	UAbilityTask_WaitGameplayEvent* WaitStruggle = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TAG_FT_Event_Struggle, /*OptionalExternalTarget=*/nullptr, /*OnlyTriggerOnce=*/false, /*OnlyMatchExact=*/true);
	WaitStruggle->EventReceived.AddDynamic(this, &UFTGA_EscapableDebuff::OnStruggleEvent);
	WaitStruggle->ReadyForActivation();
}

void UFTGA_EscapableDebuff::OnStruggleEvent(FGameplayEventData Payload)
{
	// Event.Struggle 1발 = 좌우 전환 1회(능동 탈출력). flip 판정은 입력측(플레이어)이 이미 했다.
	Gauge.AddFlip();
	if (!Gauge.IsFull())
	{
		return;
	}

	// 가득 참 → Escapable을 부여한 GE(들) 제거 → 상태 해제. 트리거 태그가 사라져 이 어빌리티도 종료된다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_FT_State_Escapable));
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
