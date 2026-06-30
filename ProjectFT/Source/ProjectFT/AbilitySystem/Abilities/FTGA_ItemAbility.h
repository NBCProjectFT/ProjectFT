// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "ProjectFT/Struct/FTItemUseStruct.h"
#include "FTGA_ItemAbility.generated.h"

class UFTItemDataAsset;
class UGameplayEffect;

/**
 * 아이템 데이터(FTItemUseStruct)로 구동되는 "사용 어빌리티"의 공용 베이스.
 * 페이로드에서 아이템 읽기 + 효과 적용(SetByCaller) + 쿨다운 인프라 + 소비 훅을 한곳에 모은다.
 * 자식은 "어떻게 적용하나"만 구현한다(중복 없이 고유 로직에 집중).
 *  - 자신에게 적용:     UFTGA_UseItem
 *  - 트레이스 대상에게: UFTGA_Taser
 *  - 투사체로 던져서:   UFTGA_ThrowItem
 */
UCLASS(Abstract)
class PROJECTFT_API UFTGA_ItemAbility : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_ItemAbility();

	// 사용 데이터의 쿨다운 태그를 해석한다(지정 태그, 없으면 공용 폴백 Cooldown.ItemUse).
	// 쿨다운 부여(ApplyCooldown)와 차단 검사(호출측)가 동일한 태그를 쓰도록 공유하는 헬퍼.
	static FGameplayTag ResolveCooldownTag(const FTItemUseStruct& UseData);

protected:
	// 쿨다운 지속시간을 ActiveUseData.CooldownSeconds로 주입한다(공용 UFTGE_Cooldown + SetByCaller). 0이면 no-op.
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 발동 페이로드에서 아이템을 읽어 ActiveUseData에 캐싱한다. 아이템이 없으면 nullptr 반환(자식이 취소 판단).
	// 반환 포인터로 메시 등 전체 데이터에 접근 가능(효과/수치/시전/쿨다운은 ActiveUseData로 충분).
	const UFTItemDataAsset* CacheActiveItem(const FGameplayEventData* TriggerEventData);

	// ActiveUseData의 UseEffects를 EffectMagnitudes(SetByCaller)로 적용한다.
	// TargetData가 nullptr이면 자신(Owner)에게, 있으면 그 대상에게 적용.
	void ApplyUseEffects(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayAbilityTargetDataHandle* TargetData = nullptr);

	// 효과 적용 직후 1회 호출되는 확장 훅(인벤토리 차감/사용 연출 등). 기본 구현은 비어 있다.
	virtual void OnItemConsumed();

	// 쿨다운 GE 클래스(아이템별 동적 태그를 얹어 어빌리티가 '직접' 적용). 기본값 UFTGE_Cooldown.
	// 표준 CooldownGameplayEffectClass 경로를 쓰지 않는 이유: 그 경로의 CheckCooldown은 GE의 '정적' 부여 태그로만
	// 차단해 어빌리티 단위(=모든 아이템 공유)로만 동작한다. 아이템별 독립 쿨다운엔 호출측(AFTPlayerCharacter) 개별 판정이 필요하다.
	UPROPERTY(EditDefaultsOnly, Category = "FT|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	// 이번 활성에서 사용할 아이템의 사용 데이터(발동 시 페이로드에서 복사). ApplyCooldown/ApplyUseEffects가 참조한다.
	UPROPERTY()
	FTItemUseStruct ActiveUseData;

	// 이번 활성에서 사용한 아이템 식별자(발동 시 페이로드에서 복사). OnItemConsumed의 인벤토리 차감에 사용한다.
	UPROPERTY()
	FName ActiveItemId = NAME_None;
};
