// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "FTGA_Example.generated.h"

/**
 * [레퍼런스 예제] GameplayAbility의 에디터 Details 패널에서 설정 가능한 모든 항목을
 * C++ 생성자에서 설정하는 방법을 보여주는 어빌리티. 실제 게임 로직은 없고,
 * 각 프로퍼티의 의미와 설정 구문을 주석으로 설명하는 것이 목적이다.
 *
 * ★ UE 5.5+ 주의: AbilityTags(=AssetTags)는 deprecated 되어 직접 대입이 금지된다.
 *   생성자에서는 반드시 SetAssetTags()로 설정해야 한다(런타임 변경 불가).
 *
 * 참고: 프로젝트 표준대로 공용 베이스 UFTGameplayAbility를 상속한다.
 *   (베이스 생성자가 InstancingPolicy = InstancedPerActor 를 이미 설정하지만,
 *    예제에서는 완결성을 위해 모든 정책을 명시적으로 다시 설정한다.)
 */
UCLASS()
class PROJECTFT_API UFTGA_Example : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_Example();

	// CommitAbility(=Cost/Cooldown 적용) → 종료까지의 최소 흐름만 보여주는 데모 구현.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
