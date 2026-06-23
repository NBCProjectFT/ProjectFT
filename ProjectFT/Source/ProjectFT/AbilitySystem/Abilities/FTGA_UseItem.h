// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_ItemAbility.h"
#include "FTGA_UseItem.generated.h"

/**
 * 아이템 사용 어빌리티(범용, 자신에게 적용). GameplayEvent(Event.UseItem)로 발동되며,
 * 페이로드의 아이템에서 사용 데이터를 읽어 (선택)시전 대기 → UseEffects를 자신에게 적용 → 쿨다운 → 종료한다.
 * 데이터 읽기/효과 적용/쿨다운 인프라는 베이스 UFTGA_ItemAbility가 제공한다 — 여기선 시전 흐름만 다룬다.
 */
UCLASS()
class PROJECTFT_API UFTGA_UseItem : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_UseItem();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	// 시전 완료(또는 시전시간 0) 시 효과 적용 + 쿨다운 적용 + 종료.
	void FinishUse();

	// 시전 대기(UAbilityTask_WaitDelay) 완료 콜백.
	UFUNCTION()
	void OnCastFinished();
};
