// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_EscapableDebuff.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Engine/World.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Character/FTCharacterBase.h"

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

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AutoEscapeDurationSeconds = ResolveAutoEscapeDurationSeconds(ASC);
	PassiveEscapeGainPerSecond = AutoEscapeDurationSeconds > KINDA_SMALL_NUMBER
		? EscapeThreshold / AutoEscapeDurationSeconds
		: 0.0f;

	// 공용 게이지 시작: 자동 해제 시간 동안 자연 증가하고, 좌우 연타가 추가로 가속한다.
	Gauge.PassiveGainPerSecond = PassiveEscapeGainPerSecond;
	Gauge.GainPerFlip = PassiveEscapeGainPerSecond * SecondsReducedPerStruggleInput;
	Gauge.Begin(EscapeThreshold, /*DecayPerSecond=*/0.0f);
	StartEscapeTick();

	// 발버둥 입력(Event.Struggle)을 계속 수신(OnlyTriggerOnce=false).
	UAbilityTask_WaitGameplayEvent* WaitStruggle = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TAG_FT_Event_Struggle, /*OptionalExternalTarget=*/nullptr, /*OnlyTriggerOnce=*/false, /*OnlyMatchExact=*/true);
	WaitStruggle->EventReceived.AddDynamic(this, &UFTGA_EscapableDebuff::OnStruggleEvent);
	WaitStruggle->ReadyForActivation();
}

void UFTGA_EscapableDebuff::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EscapeTickTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UFTGA_EscapableDebuff::OnStruggleEvent(FGameplayEventData Payload)
{
	// Event.Struggle 1발 = 좌우 전환 1회(능동 탈출력). flip 판정은 입력측(플레이어)이 이미 했다.
	Gauge.AddFlip();
	if (AFTCharacterBase* Character = Cast<AFTCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Character->PlayStruggleJitter();
	}
	TryCompleteEscape();
}

float UFTGA_EscapableDebuff::GetRemainingEscapeTime() const
{
	if (Gauge.IsFull())
	{
		return 0.0f;
	}

	if (PassiveEscapeGainPerSecond <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float RemainingStruggle = FMath::Max(0.0f, Gauge.GetThreshold() - Gauge.GetAccumulated());
	return RemainingStruggle / PassiveEscapeGainPerSecond;
}

bool UFTGA_EscapableDebuff::GetActiveEscapableDebuffInfo(
	AActor* TargetActor,
	float& OutProgress,
	float& OutRemainingTime,
	float& OutTotalTime,
	float& OutAccumulatedStruggle,
	float& OutEscapeThreshold)
{
	OutProgress = 0.0f;
	OutRemainingTime = 0.0f;
	OutTotalTime = 0.0f;
	OutAccumulatedStruggle = 0.0f;
	OutEscapeThreshold = 0.0f;

	const UFTGA_EscapableDebuff* Ability = FindActiveEscapableDebuffAbility(TargetActor);
	if (!Ability)
	{
		return false;
	}

	OutProgress = Ability->GetEscapeProgress();
	OutRemainingTime = Ability->GetRemainingEscapeTime();
	OutTotalTime = Ability->GetTotalEscapeTime();
	OutAccumulatedStruggle = Ability->GetAccumulatedStruggle();
	OutEscapeThreshold = Ability->GetEscapeThreshold();
	return true;
}

void UFTGA_EscapableDebuff::TickEscapeGauge()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(TAG_FT_State_Escapable))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	const float DeltaSeconds = LastEscapeTickTime > 0.0f
		? FMath::Max(0.0f, Now - LastEscapeTickTime)
		: EscapeTickInterval;
	LastEscapeTickTime = Now;

	Gauge.Advance(DeltaSeconds);
	TryCompleteEscape();
}

void UFTGA_EscapableDebuff::TryCompleteEscape()
{
	if (!Gauge.IsFull())
	{
		return;
	}

	RemoveEscapableEffects();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UFTGA_EscapableDebuff::RemoveEscapableEffects()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_FT_State_Escapable));
	}
}

void UFTGA_EscapableDebuff::StartEscapeTick()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	LastEscapeTickTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(
		EscapeTickTimerHandle,
		this,
		&ThisClass::TickEscapeGauge,
		EscapeTickInterval,
		true);
}

float UFTGA_EscapableDebuff::ResolveAutoEscapeDurationSeconds(const UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		return FallbackAutoEscapeSeconds;
	}

	FGameplayTagContainer Tags;
	Tags.AddTag(TAG_FT_State_Escapable);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAllOwningTags(Tags);
	const TArray<TPair<float, float>> Times = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	float BestDuration = 0.0f;
	for (const TPair<float, float>& Time : Times)
	{
		if (Time.Value > BestDuration)
		{
			BestDuration = Time.Value;
		}
	}

	return BestDuration > KINDA_SMALL_NUMBER ? BestDuration : FallbackAutoEscapeSeconds;
}

UFTGA_EscapableDebuff* UFTGA_EscapableDebuff::FindActiveEscapableDebuffAbility(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return nullptr;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!ASC || !ASC->HasMatchingGameplayTag(TAG_FT_State_Escapable))
	{
		return nullptr;
	}

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(StaticClass());
	if (!Spec)
	{
		return nullptr;
	}

	if (UFTGA_EscapableDebuff* PrimaryInstance = Cast<UFTGA_EscapableDebuff>(Spec->GetPrimaryInstance()))
	{
		return PrimaryInstance;
	}

	for (UGameplayAbility* Instance : Spec->GetAbilityInstances())
	{
		if (UFTGA_EscapableDebuff* EscapableAbility = Cast<UFTGA_EscapableDebuff>(Instance))
		{
			return EscapableAbility;
		}
	}

	return nullptr;
}
