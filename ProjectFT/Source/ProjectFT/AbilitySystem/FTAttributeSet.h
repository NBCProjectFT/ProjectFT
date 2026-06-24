// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "FTAttributeSet.generated.h"

// 표준 ATTRIBUTE_ACCESSORS(GAS 샘플 관용): Attribute getter + 값 getter/setter/initter 일괄 생성.
// FTPlayerAttributeSet.h와 동일 정의 — 두 헤더가 한 TU에 함께 포함돼도 중복 정의되지 않도록 가드한다.
#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

/**
 * 캐릭터 공용 스탯을 GAS 속성으로 보유한다(싱글이라 비복제). 플레이어/AI가 함께 쓰는 기본 스탯만 둔다.
 * 기본값/클램프/사망 판정을 담당하며, 플레이어 전용 스탯(스태미나·이동 배수·손재주)은 UFTPlayerAttributeSet에 있다.
 * 버프 포함 최종값은 GAS 어그리게이터가 산출하며, GetX()가 그 현재값을 돌려준다.
 */
UCLASS()
class PROJECTFT_API UFTAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UFTAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UFTAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "FT|Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UFTAttributeSet, MaxHealth)

	// 기본 이동속도(cm/s). 버프는 이 속성에 모디파이어로 얹힌다.
	UPROPERTY(BlueprintReadOnly, Category = "FT|Attributes")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UFTAttributeSet, MoveSpeed)

	// 체력이 0에 도달했을 때 1회 통지(캐릭터가 바인딩해 사망 처리).
	mutable FSimpleMulticastDelegate OnOutOfHealth;
};
