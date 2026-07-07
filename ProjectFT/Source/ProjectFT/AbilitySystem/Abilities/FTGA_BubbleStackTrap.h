// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FTGameplayAbility.h"
#include "FTGA_BubbleStackTrap.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * 비눗방울 피격 스택이 임계치에 도달하면 '갇힘' 상태(UFTGE_BubbleTrap)를 적용하는 어빌리티.
 *
 * 트리거: State.Debuff.BubbleStack이 붙는 순간(OwnedTagPresent) 자동 발동. 활성 동안 스택 카운트를 감시하다가
 * BubbleTrapThreshold에 도달하면 갇힘 GE(지속시간은 Stack GE에 실려온 Data.BubbleDuration)를 자기 자신에게 적용하고, 쌓인 스택 GE를 제거한다.
 * 스택이 제거되면 트리거 태그가 사라져 어빌리티는 자동 종료된다(다음 스택이 다시 쌓이면 재발동).
 *
 * 갇힘 자체(타이머/이동정지/비주얼)는 전부 GE가 담당하고, 이 어빌리티는 "언제 갇힐지"만 판정한다.
 */
UCLASS()
class PROJECTFT_API UFTGA_BubbleStackTrap : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_BubbleStackTrap();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 갇힘으로 전환되는 데 필요한 동시 스택 수(= State.Debuff.BubbleStack 카운트 임계치).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Bubble", meta = (ClampMin = "1"))
	int32 BubbleTrapThreshold = 5;

	// 적용할 갇힘 상태 GE. 기본은 UFTGE_BubbleTrap(에디터에서 교체 가능).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Bubble")
	TSubclassOf<UGameplayEffect> TrapEffectClass;

	// Stack GE에 Data.BubbleDuration이 없을 때 쓸 안전 기본값. DA 설정 누락으로 Trap GE가 즉시 만료되는 것을 막는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Bubble", meta = (ClampMin = "0.0"))
	float DefaultBubbleDuration = 5.0f;

private:
	// 스택 카운트 변화 콜백 → 임계치 도달 시 갇힘 GE 적용.
	void OnStackCountChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// 임계치 도달 시 1회: 갇힘 GE 적용 + 쌓인 스택 제거.
	void TryTrap(int32 CurrentStackCount);

	float ResolveBubbleDurationFromActiveStacks(const UAbilitySystemComponent* ASC) const;

	FDelegateHandle StackCountHandle;
};
