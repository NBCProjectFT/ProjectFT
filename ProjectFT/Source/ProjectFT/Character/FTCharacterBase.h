// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "FTCharacterBase.generated.h"

class UAbilitySystemComponent;
class UFTAttributeSet;
struct FOnAttributeChangeData;

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

	// MoveSpeed 속성(버프 포함 최종값)을 CMC의 MaxWalkSpeed에 반영한다. 기본 구현은 MaxWalkSpeed = MoveSpeed.
	// 스프린트/앉기 등 추가 연산이 필요한 자식은 이 함수를 override 한다.
	virtual void ApplyMovementSpeed();

	// 이동속도 관련 속성이 바뀌면(Slow/Haste 등) ApplyMovementSpeed를 다시 적용한다. 자식이 추가 속도 속성에도 바인딩할 수 있다.
	void OnSpeedAttributeChanged(const FOnAttributeChangeData& Data);

	// 스턴 태그(State.Debuff.Stun) 부착/해제 시 호출(BeginPlay에서 등록). 공통 이동 정지/복원 후 OnStunStateChanged 훅을 부른다.
	void OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// 스턴 시작/해제 시 확장 훅(AI 로직 정지, 애니 등). 기본 구현 없음 — 이동 정지/복원은 베이스가 이미 처리.
	virtual void OnStunStateChanged(bool bStunned);

	// GAS: 능력/이펙트/속성의 허브. 공용 속성값(체력/이동속도)은 AttributeSet이 보유한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 캐릭터의 서브오브젝트로 만들면 ASC가 InitializeComponent 시 자동 등록한다(공용 스탯: 체력/이동속도).
	UPROPERTY()
	TObjectPtr<UFTAttributeSet> AttributeSet;
};
