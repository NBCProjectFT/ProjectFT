// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "FTGameplayAbility.generated.h"

/**
 * 프로젝트 공용 GameplayAbility 베이스. 인스턴싱/실행 정책 등 공통 기본값을 모은다.
 */
UCLASS(Abstract)
class PROJECTFT_API UFTGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGameplayAbility();

	// 이 어빌리티가 GameplayEvent로 발동될 때의 트리거 태그(첫 GameplayEvent 트리거). 캐릭터가 사용 시 이 태그로 이벤트를 보낸다.
	UFUNCTION(BlueprintCallable, Category = "FT|Ability")
	FGameplayTag GetTriggerEventTag() const;

	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToOwnerPublic(
		const FGameplayAbilitySpecHandle AbilityHandle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEffectSpecHandle SpecHandle) const;

	TArray<FActiveGameplayEffectHandle> ApplyGameplayEffectSpecToTargetPublic(
		const FGameplayAbilitySpecHandle AbilityHandle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEffectSpecHandle SpecHandle,
		const FGameplayAbilityTargetDataHandle& TargetData) const;
};
