// Fill out your copyright notice in the Description page of Project Settings.

#include "FTCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"

AFTCharacterBase::AFTCharacterBase()
{
	// GAS: 능력시스템 컴포넌트 + 공용 속성셋. 속성셋은 캐릭터 서브오브젝트라 ASC가 자동 등록한다.
	// (서브클래스가 추가 속성셋을 더 만들면 그 세트도 같은 ASC에 자동 등록된다.)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UFTAttributeSet>(TEXT("AttributeSet"));

	// CMC의 초기 MaxWalkSpeed(CDO/프리뷰)를 MoveSpeed 속성 기본값에서 가져온다 — 자식이 ctor에서 따로 셋하지 않게 단일화.
	// (런타임엔 BeginPlay의 ApplyMovementSpeed가 최종값으로 다시 덮어쓴다.)
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
	}

	// 무기 트레이스(Weapon = ECC_GameTraceChannel1)가 캡슐을 '통과'해 메시에 정밀히 맞도록 캡슐만 Ignore로 둔다.
	// 캡슐 기본 Weapon 응답은 Block이라 명시적으로 덮어씀 — 메시는 CharacterMesh 프로파일(QueryOnly + Weapon 기본 Block)이라 그대로 맞는다.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	}
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

		// MoveSpeed 속성 → MaxWalkSpeed 반영(Slow/Haste가 이동에 보이도록). 모든 캐릭터 공통 기본 파생을 베이스가 제공한다.
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &AFTCharacterBase::OnSpeedAttributeChanged);

		// 스턴 태그 부착/해제 → 공통 이동 정지/복원. 모든 캐릭터 동일하므로 베이스가 처리하고, 추가 반응은 OnStunStateChanged override.
		AbilitySystemComponent->RegisterGameplayTagEvent(TAG_FT_State_Debuff_Stun, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &AFTCharacterBase::OnStunTagChanged);
	}

	if (AttributeSet)
	{
		// 체력 0 도달 시 HandleDeath()로 통지 — 서브클래스가 사망 처리.
		AttributeSet->OnOutOfHealth.AddUObject(this, &AFTCharacterBase::HandleDeath);
	}

	// 초기 MoveSpeed를 MaxWalkSpeed에 반영(자식이 override 했으면 그 구현으로).
	ApplyMovementSpeed();
}

void AFTCharacterBase::HandleDeath()
{
	// 기본 구현 없음. 서브클래스가 사망 연출/레벨 전환 요청 등을 처리한다.
}

void AFTCharacterBase::ApplyMovementSpeed()
{
	// 기본 파생: MaxWalkSpeed = MoveSpeed(버프 포함 최종값). 스프린트/앉기 등은 자식이 override.
	if (!AttributeSet)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
	}
}

void AFTCharacterBase::OnSpeedAttributeChanged(const FOnAttributeChangeData& Data)
{
	ApplyMovementSpeed();
}

void AFTCharacterBase::OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	const bool bStunned = NewCount > 0;

	// 공통 반응: 스턴 시작 시 즉시 정지+이동 비활성, 해제 시 보행 복원. (보행 캐릭터 기준 — 다른 이동 모드는 자식이 OnStunStateChanged에서 보정.)
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (bStunned)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		else
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}

	// 스턴 '지속' 연출(GameplayCue)은 GE_Stun의 GameplayCues에 GameplayCue.State.Stun을 달아
	// GE 수명과 함께 자동 발동/제거된다(여기서 직접 Add/Remove하지 않는다 — 중복 발동 방지).
	OnStunStateChanged(bStunned);
}

void AFTCharacterBase::OnStunStateChanged(bool bStunned)
{
	// 기본 구현 없음. 자식이 AI 로직 정지/애니 등 추가 반응을 처리한다(이동 정지/복원은 베이스가 이미 처리).
}
