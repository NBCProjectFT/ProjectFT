// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "FTPlayerAttributeSet.generated.h"

// 표준 ATTRIBUTE_ACCESSORS(GAS 샘플 관용): Attribute getter + 값 getter/setter/initter 일괄 생성.
// FTAttributeSet.h와 동일 정의 — 두 헤더가 한 TU에 함께 포함돼도 중복 정의되지 않도록 가드한다.
#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

/**
 * 플레이어 전용 스탯을 GAS 속성으로 보유한다(싱글이라 비복제).
 * 캐릭터 공용 스탯(체력/최대체력/이동속도)은 UFTAttributeSet에 있고, 여기엔 플레이어에게만 의미 있는 스탯만 둔다.
 * 같은 ASC에 공용 세트와 함께 등록된다(캐릭터의 서브오브젝트라 ASC가 자동 등록).
 */
UCLASS()
class PROJECTFT_API UFTPlayerAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UFTPlayerAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Attributes")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UFTPlayerAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "FT|Attributes")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UFTPlayerAttributeSet, MaxStamina)

	// 손재주. 채널형 상호작용(훔치기 등)의 작업 속도 배수로 쓰인다.
	UPROPERTY(BlueprintReadOnly, Category = "FT|Attributes")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UFTPlayerAttributeSet, Dexterity)

	// 스프린트 이동속도 배수(공용 MoveSpeed에 곱해 최종 속도 산출).
	UPROPERTY(BlueprintReadOnly, Category = "FT|Attributes")
	FGameplayAttributeData SprintSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UFTPlayerAttributeSet, SprintSpeedMultiplier)

	// 앉기 이동속도 배수(공용 MoveSpeed에 곱해 최종 속도 산출).
	UPROPERTY(BlueprintReadOnly, Category = "FT|Attributes")
	FGameplayAttributeData CrouchSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UFTPlayerAttributeSet, CrouchSpeedMultiplier)
};
