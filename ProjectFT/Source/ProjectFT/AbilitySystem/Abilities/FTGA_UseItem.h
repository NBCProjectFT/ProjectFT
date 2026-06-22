// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "FTGA_UseItem.generated.h"

class UGameplayEffect;
class UFTItemDataAsset;

/**
 * 아이템 사용 어빌리티 베이스. 활성화하면 (선택)시전시간을 대기한 뒤 ItemEffect를 자신에게 적용하고 쿨다운을 건다.
 * 효과/시전시간/쿨다운은 파생 클래스나 BP에서 지정한다.
 * (아이템 데이터 시스템이 붙으면 이 값들을 외부 데이터 참조로 대체할 예정 — 그때까지 어빌리티가 데이터를 직접 보유한다.)
 */
UCLASS()
class PROJECTFT_API UFTGA_UseItem : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_UseItem();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 쿨다운 지속시간을 CooldownSeconds로 주입하기 위해 표준 ApplyCooldown을 재정의한다.
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 효과 적용 직후 1회 호출되는 확장 훅(인벤토리 차감/사용 연출 등). 기본 구현은 비어 있다.
	virtual void OnItemConsumed();

	// 사용 시 자신에게 적용할 효과(예: UFTGE_Heal/SpeedBuff/Poison). 파생/BP에서 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Item")
	TSubclassOf<UGameplayEffect> ItemEffect;

	// 시전 시간(초). 0이면 즉시 적용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Item", meta = (ClampMin = "0.0"))
	float CastTimeSeconds = 0.0f;

	// 쿨다운(초). 0이면 쿨다운 없음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Item", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 0.0f;

private:
	const UFTItemDataAsset* GetItemData() const;
	float GetUseCastTime() const;
	float GetUseCooldown() const;
	TSubclassOf<UGameplayEffect> GetUseEffectClass() const;
	bool PlayItemMontage() const;

	// 시전 완료(또는 시전시간 0) 시 효과 적용 + 쿨다운 적용 + 종료.
	void FinishUse();

	// 시전 대기(UAbilityTask_WaitDelay) 완료 콜백.
	UFUNCTION()
	void OnCastFinished();
};
