// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "FTCharacterBase.generated.h"

class UAbilitySystemComponent;
class UFTAttributeSet;
class UGameplayAbility;
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

	// 사망 처리가 완료된 상태(체력 소진). 사망 연출/AnimBP/AI가 "죽었나?" 판정에 쓴다.
	UFUNCTION(BlueprintPure, Category = "FT|GAS")
	bool IsDead() const { return bDead; }

	// 발버둥 입력이 유효하게 처리됐을 때 메시만 짧게 흔든다. 캡슐/Actor 위치는 건드리지 않는다.
	UFUNCTION(BlueprintCallable, Category = "FT|Feedback")
	void PlayStruggleJitter();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 체력이 0에 도달했을 때 호출(공용 속성셋의 OnOutOfHealth 통지). 공통 사망 처리를 담당한다:
	// 재진입 가드(bDead) + State.Dead 태그 부여 + 진행 중 능력 취소 + 이동 정지. 이후 확장 훅 OnDeath()를 부른다.
	// (행동불능의 OnImmobilizeTagChanged/OnImmobilizedStateChanged와 같은 Template Method 패턴 — 자식은 OnDeath()만 override 한다.)
	void HandleDeath();

	// 사망 시 확장 훅. 자식이 사망 연출/후처리를 담당한다(플레이어=입력 차단/게임오버, AI=래그돌/드롭/디스폰 등).
	// 공통 처리(태그/능력취소/이동정지)는 HandleDeath가 이미 수행했고, bDead 가드로 1회만 호출됨이 보장된다. 기본 구현 없음.
	virtual void OnDeath();

	// MoveSpeed 속성(버프 포함 최종값)을 CMC의 MaxWalkSpeed에 반영한다. 기본 구현은 MaxWalkSpeed = MoveSpeed.
	// 스프린트/앉기 등 추가 연산이 필요한 자식은 이 함수를 override 한다.
	virtual void ApplyMovementSpeed();

	// 이동속도 관련 속성이 바뀌면(Slow/Haste 등) ApplyMovementSpeed를 다시 적용한다. 자식이 추가 속도 속성에도 바인딩할 수 있다.
	void OnSpeedAttributeChanged(const FOnAttributeChangeData& Data);

	// 행동불능 우산 태그(State.Debuff.Immobilized) 부착/해제 시 호출(BeginPlay에서 이 태그로 등록).
	// 우산 태그 카운트가 곧 활성 행동불능 수라, NewCount>0면 봉쇄 유지 후 OnImmobilizedStateChanged 훅을 부른다.
	void OnImmobilizeTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// 행동불능 시작/해제 시 확장 훅(AI 로직 정지, 애니 등). 기본 구현 없음 — 이동 정지/복원은 베이스가 이미 처리.
	virtual void OnImmobilizedStateChanged(bool bImmobilized);

	// GAS: 능력/이펙트/속성의 허브. 공용 속성값(체력/이동속도)은 AttributeSet이 보유한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 캐릭터의 서브오브젝트로 만들면 ASC가 InitializeComponent 시 자동 등록한다(공용 스탯: 체력/이동속도).
	UPROPERTY()
	TObjectPtr<UFTAttributeSet> AttributeSet;

	// 모든 캐릭터에 공통으로 부여할 '추가' 어빌리티(BeginPlay에서 GiveAbility). BP에서 자유롭게 추가한다.
	// 비눗방울 갇힘/탈출(UFTGA_BubbleStackTrap·UFTGA_EscapableDebuff)은 여기 넣지 않아도 베이스가 항상 보장하므로 비워둬도 된다.
	// 경비 전용 능력(잡기 등)은 여기가 아니라 AFTSecurityCharacter의 DefaultAbilities에서 부여한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS")
	TArray<TSubclassOf<UGameplayAbility>> CommonAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Feedback", meta = (ClampMin = "0.0"))
	float StruggleJitterAmplitude = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Feedback", meta = (ClampMin = "0.01"))
	float StruggleJitterDuration = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Feedback", meta = (ClampMin = "0.01"))
	float StruggleJitterFrequency = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Feedback", meta = (ClampMin = "0.005"))
	float StruggleJitterTickInterval = 0.016f;

	// 사망 처리 완료 플래그이자 재진입 가드. 0 HP에서 독 DoT 등이 계속 틱해 OnOutOfHealth가 재통지돼도 HandleDeath는 1회만 실행된다.
	bool bDead = false;

private:
	void UpdateStruggleJitter();
	void StopStruggleJitter();
	void ApplyStruggleJitterOffset(const FVector& NewOffset);

	FTimerHandle StruggleJitterTimerHandle;
	FVector StruggleJitterAppliedOffset = FVector::ZeroVector;
	float StruggleJitterElapsed = 0.0f;
	float StruggleJitterLastUpdateTime = 0.0f;
	int32 StruggleJitterDirection = 1;
};
