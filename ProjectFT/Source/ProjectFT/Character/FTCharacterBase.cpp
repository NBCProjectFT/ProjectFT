// Fill out your copyright notice in the Description page of Project Settings.

#include "FTCharacterBase.h"

#include "AbilitySystemComponent.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"

AFTCharacterBase::AFTCharacterBase()
{
	// GAS: 능력시스템 컴포넌트 + 공용 속성셋. 속성셋은 캐릭터 서브오브젝트라 ASC가 자동 등록한다.
	// (서브클래스가 추가 속성셋을 더 만들면 그 세트도 같은 ASC에 자동 등록된다.)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UFTAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AFTCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AFTCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// 싱글: 소유자=아바타=this. (InitializeComponent가 한 번 호출하지만 명시적으로 한 번 더 — 안전.)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (AttributeSet)
	{
		// 체력 0 도달 시 HandleDeath()로 통지 — 서브클래스가 사망 처리.
		AttributeSet->OnOutOfHealth.AddUObject(this, &AFTCharacterBase::HandleDeath);
	}
}

void AFTCharacterBase::HandleDeath()
{
	// 기본 구현 없음. 서브클래스가 사망 연출/레벨 전환 요청 등을 처리한다.
}
