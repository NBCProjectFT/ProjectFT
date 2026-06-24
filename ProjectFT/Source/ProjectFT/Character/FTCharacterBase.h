// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "FTCharacterBase.generated.h"

class UAbilitySystemComponent;
class UFTAttributeSet;

/**
 * 플레이어/AI가 공유하는 캐릭터 베이스. GAS '배선(plumbing)'만 담는다.
 *  - ASC + 공용 속성셋(UFTAttributeSet: 체력/최대체력/이동속도) 생성·소유
 *  - IAbilitySystemInterface 구현 + BeginPlay에서 InitAbilityActorInfo
 *  - 체력 소진 시 HandleDeath() 훅 호출(서브클래스가 사망 처리)
 * 캐릭터별 행동(입력/카메라/이동 파생/AI/플레이어 전용 속성)은 서브클래스가 담당한다.
 */
UCLASS(Abstract)
class PROJECTFT_API AFTCharacterBase : public ACharacter, public IFTDamageable, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AFTCharacterBase();

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

protected:
	virtual void BeginPlay() override;

	// 체력이 0에 도달했을 때 호출(공용 속성셋의 OnOutOfHealth 통지). 기본 구현은 비어 있고, 서브클래스가 사망 처리한다.
	virtual void HandleDeath();

	// GAS: 능력/이펙트/속성의 허브. 공용 속성값(체력/이동속도)은 AttributeSet이 보유한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 캐릭터의 서브오브젝트로 만들면 ASC가 InitializeComponent 시 자동 등록한다(공용 스탯: 체력/이동속도).
	UPROPERTY()
	TObjectPtr<UFTAttributeSet> AttributeSet;
};
