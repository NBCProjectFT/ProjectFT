// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "FTCharacterBase.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
class UFTAttributeSet;
class UGameplayAbility;
struct FOnAttributeChangeData;
struct FGameplayEffectSpec;
struct FActiveGameplayEffectHandle;

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

	// 행동불능(우산 태그 State.Debuff.Immobilized 보유) 상태인지. AnimBP가 행동불능 스테이트 진입/이탈 판정에 쓴다.
	UFUNCTION(BlueprintPure, Category = "FT|Debuff")
	bool IsImmobilized() const { return bIsImmobilized; }

	// 지금 재생해야 할 행동불능 '포즈'를 대표하는 태그(예: State.Debuff.Stun).
	// 여러 행동불능이 겹쳤을 땐 ImmobilizePosePriority 순서로 하나만 고른다. 행동불능이 아니면 빈 태그.
	// AnimBP는 이 값 하나만 비교해서 스턴/마비/비눗방울 포즈를 분기하면 된다.
	// 캐시하지 않고 호출 시점에 ASC를 조회한다 — 하나의 GE가 우산 태그와 개별 태그(Stun 등)를 함께 부여할 때
	// 태그 추가 순서가 보장되지 않아, 우산 태그 콜백 시점에 캐시하면 개별 태그를 놓칠 수 있기 때문.
	UFUNCTION(BlueprintPure, Category = "FT|Debuff")
	FGameplayTag GetActiveImmobilizePoseTag() const;

	// 발버둥 입력이 유효하게 처리됐을 때 메시만 짧게 흔든다. 캡슐/Actor 위치는 건드리지 않는다.
	UFUNCTION(BlueprintCallable, Category = "FT|Feedback")
	void PlayStruggleJitter();

	// 피격 반응 몽타주를 재생한다. 적대적 GE를 맞으면 OnHostileEffectApplied가 자동으로 부르지만,
	// GE를 거치지 않는 연출(스크립트 이벤트 등)에서 BP가 직접 호출할 수도 있다.
	// EffectTags는 몽타주 선택 분기용 — 비워서 호출하면 SelectHitReactMontage의 기본값(HitReactMontage)이 쓰인다.
	UFUNCTION(BlueprintCallable, Category = "FT|Feedback")
	void PlayHitReact(const FGameplayTagContainer& EffectTags);

	// 공격 종류(에셋 태그 + 부여 태그)에 따라 재생할 피격 몽타주를 고르는 확장 지점.
	// 기본 구현은 종류 불문 HitReactMontage. 스턴/화염 등 반응을 나누고 싶으면 BP나 자식에서 override 한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "FT|Feedback")
	UAnimMontage* SelectHitReactMontage(const FGameplayTagContainer& EffectTags) const;
	virtual UAnimMontage* SelectHitReactMontage_Implementation(const FGameplayTagContainer& EffectTags) const;

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

	// 행동불능이 겹쳤을 때 어떤 포즈가 이기는지의 순서. 위에 있을수록 우선(예: 잡힘 > 스턴 > 비눗방울).
	// 여기 없는(또는 배열이 빈) 행동불능은 우산 태그 State.Debuff.Immobilized로 폴백되므로,
	// AnimBP에 '공용 행동불능' 스테이트 하나만 있으면 새 상태이상이 추가돼도 포즈가 비지 않는다.
	// 생성자 기본값은 기존 BP 인스턴스에 전파가 불안정해서 비워둔다 — 값은 BP에서 채울 것.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Debuff")
	TArray<FGameplayTag> ImmobilizePosePriority;

	// 우산 태그 카운트에서 갱신되는 행동불능 여부 캐시. 쓰기는 OnImmobilizeTagChanged 단독이다.
	UPROPERTY(BlueprintReadOnly, Category = "FT|Debuff")
	bool bIsImmobilized = false;

	// GE가 자신에게 적용될 때마다 호출(BeginPlay에서 ASC의 OnGameplayEffectAppliedDelegateToSelf에 바인딩 — instant/duration 모두).
	// 적대적 행동(Effect.Hostile 에셋 태그)을 부여하는 GE면 데미지/상태이상 구분 없이 "공격당함"으로 간주해,
	// 공격자(EffectContext에서 추출, 더미 속성 불필요)를 담아 Event.Character.Attacked를 발행한다. 어그로는 이 단일 신호를 구독한다.
	void OnHostileEffectApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

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

	// 적대적 GE(Effect.Hostile)를 맞았을 때 재생할 기본 피격 몽타주. 비워두면 피격 반응 없음(기존 동작 유지).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Feedback")
	TObjectPtr<UAnimMontage> HitReactMontage = nullptr;

	// 피격 반응 재생 최소 간격(초). 산탄/다단히트로 몽타주가 매 히트마다 처음부터 재시작해
	// 제자리에서 떠는 것을 막는다. 0이면 제한 없음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Feedback", meta = (ClampMin = "0.0"))
	float HitReactMinInterval = 0.35f;

	// true면 행동불능(잡힘/비눗방울 등) 중에도 피격 반응을 재생한다. 기본은 false —
	// 구속 연출이 도는 중에 피격 몽타주가 상체를 덮어써 자세가 풀려 보이기 때문.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Feedback")
	bool bPlayHitReactWhileImmobilized = false;

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

	// 마지막으로 피격 몽타주를 재생한 월드 시각. HitReactMinInterval 판정용
	// (월드 시각은 0부터 시작하므로, 큰 음수로 두면 첫 피격은 간격 검사를 항상 통과한다).
	float LastHitReactTime = -1000.0f;

	FTimerHandle StruggleJitterTimerHandle;
	FVector StruggleJitterAppliedOffset = FVector::ZeroVector;
	float StruggleJitterElapsed = 0.0f;
	float StruggleJitterLastUpdateTime = 0.0f;
	int32 StruggleJitterDirection = 1;

	bool bHasPreImmobilizedMovementMode = false;
	TEnumAsByte<EMovementMode> PreImmobilizedMovementMode = MOVE_None;
	uint8 PreImmobilizedCustomMovementMode = 0;
};
