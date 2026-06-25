#pragma once

#include "CoreMinimal.h"
#include "FTGameplayAbility.h"
#include "FTGA_MeleeAttack.generated.h"

class UFTItemDataAsset;
class UFTMeleeDataAsset;
class USkeletalMeshComponent;
struct FFTMeleeAttackStruct;

/**
 * UFTGA_MeleeAttack
 *
 * 근접 공격 전용 GameplayAbility입니다.
 *
 * 이 Ability의 핵심 역할:
 *
 * 1. GameplayEvent Payload로 넘어온 ItemData를 받습니다.
 * 2. 그 ItemData가 UFTMeleeDataAsset인지 확인합니다.
 * 3. UFTMeleeDataAsset 안의 MeleeAttackData를 기준으로 공격 몽타주를 재생합니다.
 * 4. 몽타주 안에 배치된 AnimNotifyState가 공격 판정 시작/진행/종료 함수를 호출합니다.
 * 5. 공격 판정 구간 동안 HitStartSocketName / HitEndSocketName을 기준으로 캡슐 오버랩을 수행합니다.
 * 6. 한 번의 공격에서 이미 맞은 대상은 HitActors에 저장해서 중복 타격을 막습니다.
 * 7. 맞은 대상의 ASC를 찾아 ItemData.UseData.UseEffects를 적용합니다.
 *
 * 구조상 책임 분리:
 *
 * - EquipmentComponent / QuickSlot:
 *   어떤 아이템을 사용할지 결정하고 GameplayEvent를 보냅니다.
 *
 * - UFTGA_MeleeAttack:
 *   공격 실행, 몽타주 재생, 공격 판정, 효과 적용을 담당합니다.
 *
 * - AnimNotifyState:
 *   직접 판정하지 않습니다.
 *   단지 몽타주의 특정 타이밍에서 Ability의 StartMeleeTrace / PerformMeleeTrace / StopMeleeTrace를 호출합니다.
 *
 * - ItemData / MeleeDataAsset:
 *   공격에 필요한 데이터, 몽타주, 소켓 이름, 캡슐 크기, 디버그 여부, 적용할 GameplayEffect를 가집니다.
 */
UCLASS()
class PROJECTFT_API UFTGA_MeleeAttack : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_MeleeAttack();

	/**
	 * GAS가 Ability를 실행할 때 호출하는 함수입니다.
	 *
	 * 이 Ability는 직접 TryActivateAbility로 실행되기보다,
	 * ASC->HandleGameplayEvent()로 들어온 GameplayEvent에 의해 실행됩니다.
	 *
	 * 현재 흐름:
	 *
	 * 1. TriggerEventData->OptionalObject에서 UFTItemDataAsset을 꺼냅니다.
	 * 2. 해당 ItemData를 UFTMeleeDataAsset으로 Cast합니다.
	 * 3. HitActors와 Trace 상태를 초기화합니다.
	 * 4. CommitAbility로 쿨다운/코스트/태그 조건을 확정합니다.
	 * 5. MeleeAttackData를 가져옵니다.
	 * 6. AttackMontage가 있으면 AbilityTask_PlayMontageAndWait로 재생합니다.
	 * 7. 몽타주가 끝나거나 중단되면 Ability를 종료합니다.
	 */
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	/**
	 * 근접 공격 판정을 시작합니다.
	 *
	 * 호출 위치:
	 * - 보통 AnimNotifyState의 NotifyBegin에서 호출합니다.
	 *
	 * 동작:
	 * - 이전 공격에서 기록된 HitActors를 초기화합니다.
	 * - bTraceActive를 true로 바꿔서 PerformMeleeTrace가 동작할 수 있게 합니다.
	 * - NotifyBegin이 들어온 바로 그 프레임도 놓치지 않도록 PerformMeleeTrace를 한 번 즉시 호출합니다.
	 *
	 * 이유:
	 * - 빠른 공격 모션에서는 NotifyBegin과 첫 Tick 사이에 이미 타격 위치가 지나갈 수 있습니다.
	 * - 그래서 시작 순간에 한 번 바로 검사하는 것이 안전합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Melee")
	void StartMeleeTrace();

	/**
	 * 실제 근접 공격 판정을 수행합니다.
	 *
	 * 호출 위치:
	 * - 보통 AnimNotifyState의 NotifyTick에서 매 프레임 호출합니다.
	 *
	 * 동작:
	 * - bTraceActive가 true일 때만 동작합니다.
	 * - 현재 MeleeAttackData를 가져옵니다.
	 * - 공격자의 SkeletalMeshComponent를 찾습니다.
	 * - HitStartSocketName / HitEndSocketName 위치를 기준으로 캡슐 정보를 만듭니다.
	 * - World->OverlapMultiByChannel()로 캡슐 오버랩을 수행합니다.
	 * - 이미 맞은 대상은 HitActors로 걸러냅니다.
	 * - 처음 맞은 대상에게 ItemData.UseData.UseEffects를 적용합니다.
	 * - bDrawDebug가 true면 판정 캡슐을 화면에 그립니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Melee")
	void PerformMeleeTrace();

	/**
	 * 근접 공격 판정을 종료합니다.
	 *
	 * 호출 위치:
	 * - 보통 AnimNotifyState의 NotifyEnd에서 호출합니다.
	 *
	 * 동작:
	 * - bTraceActive를 false로 바꿔 PerformMeleeTrace가 더 이상 동작하지 않게 합니다.
	 * - HitActors를 비웁니다.
	 *
	 * 주의:
	 * - HitActors를 여기서 비우면 다음 공격 때 다시 같은 대상을 때릴 수 있습니다.
	 * - 한 번의 공격 중 중복 타격만 막는 구조입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Melee")
	void StopMeleeTrace();

private:
	/**
	 * 몽타주가 정상 완료되었거나 BlendOut 되었을 때 호출됩니다.
	 *
	 * 현재 cpp에서는:
	 * - OnCompleted
	 * - OnBlendOut
	 *
	 * 둘 다 이 함수에 연결되어 있습니다.
	 *
	 * 동작:
	 * - EndMeleeAbility(false)를 호출해서 정상 종료 처리합니다.
	 */
	UFUNCTION()
	void HandleMontageCompleted();

	/**
	 * 몽타주가 중단되었거나 취소되었을 때 호출됩니다.
	 *
	 * 현재 cpp에서는:
	 * - OnInterrupted
	 * - OnCancelled
	 *
	 * 둘 다 이 함수에 연결되어 있습니다.
	 *
	 * 동작:
	 * - EndMeleeAbility(true)를 호출해서 취소 종료 처리합니다.
	 */
	UFUNCTION()
	void HandleMontageInterrupted();

	/**
	 * 근접 공격 Ability를 종료하는 공통 함수입니다.
	 *
	 * 동작:
	 * - StopMeleeTrace()를 호출해서 공격 판정 상태를 정리합니다.
	 * - EndAbility()를 호출해서 GAS에게 Ability 종료를 알립니다.
	 *
	 * bWasCancelled:
	 * - true이면 취소/중단 종료
	 * - false이면 정상 종료
	 */
	void EndMeleeAbility(bool bWasCancelled);

	/**
	 * 현재 활성화된 근접 공격 데이터를 가져옵니다.
	 *
	 * ActiveMeleeData가 있으면 그 안의 MeleeAttackData 주소를 반환합니다.
	 * 없으면 nullptr을 반환합니다.
	 *
	 * 이 함수가 반환하는 데이터에는 보통 이런 값들이 들어 있습니다.
	 *
	 * - AttackMontage
	 * - HitStartSocketName
	 * - HitEndSocketName
	 * - CapsuleRadius
	 * - TraceChannel
	 * - bDrawDebug
	 * - AttachSocketName 등
	 */
	const FFTMeleeAttackStruct* GetMeleeAttackData() const;

	/**
	 * 공격 판정에 사용할 SkeletalMeshComponent를 찾습니다.
	 *
	 * 우선순위:
	 * 1. CurrentActorInfo->SkeletalMeshComponent
	 * 2. AvatarActor 안에서 USkeletalMeshComponent 검색
	 *
	 * 이유:
	 * - GAS ActorInfo에 SkeletalMeshComponent가 들어있는 경우가 있습니다.
	 * - 없을 수도 있으므로 AvatarActor에서 fallback 검색합니다.
	 */
	USkeletalMeshComponent* ResolveSourceMesh() const;

	/**
	 * HitStartSocketName / HitEndSocketName을 기준으로 캡슐 판정 정보를 만듭니다.
	 *
	 * OutStart:
	 * - 시작 소켓의 월드 위치
	 *
	 * OutEnd:
	 * - 끝 소켓의 월드 위치
	 *
	 * OutHalfHeight:
	 * - OverlapMultiByChannel에 사용할 캡슐 HalfHeight
	 *
	 * OutRotation:
	 * - 캡슐이 Start -> End 방향을 바라보도록 하는 회전값
	 *
	 * 반환값:
	 * - true: 캡슐 정보를 정상적으로 만들었음
	 * - false: Mesh 없음, MeleeData 없음, 소켓 없음, 소켓 간 거리가 너무 짧음
	 */
	bool BuildTraceCapsule(
		USkeletalMeshComponent* SourceMesh,
		FVector& OutStart,
		FVector& OutEnd,
		float& OutHalfHeight,
		FQuat& OutRotation
	) const;

	/**
	 * 타격 대상에게 ItemData.UseData.UseEffects를 적용합니다.
	 *
	 * 동작:
	 * - SourceASC를 가져옵니다.
	 * - TargetActor의 ASC를 가져옵니다.
	 * - ActiveItemData->ItemData.UseData.UseEffects 목록을 순회합니다.
	 * - 각 GameplayEffect Spec을 만듭니다.
	 * - UseData.EffectMagnitudes에 있는 SetByCaller 값을 Spec에 넣습니다.
	 * - SourceASC->ApplyGameplayEffectSpecToTarget()으로 대상에게 적용합니다.
	 *
	 * 주의:
	 * - TargetActor가 ASC를 가지고 있지 않으면 효과 적용이 스킵됩니다.
	 * - UseEffects가 비어 있으면 아무 효과도 적용되지 않습니다.
	 */
	void ApplyItemEffectsToTarget(AActor* TargetActor) const;

	/**
	 * 이번 Ability 실행에서 사용 중인 원본 ItemData입니다.
	 *
	 * ActivateAbility에서 TriggerEventData->OptionalObject를 통해 들어옵니다.
	 *
	 * 사용 위치:
	 * - ApplyItemEffectsToTarget()
	 * - 로그 출력
	 * - UseData.UseEffects / EffectMagnitudes 참조
	 */
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;

	/**
	 * ActiveItemData를 UFTMeleeDataAsset으로 Cast한 결과입니다.
	 *
	 * 근접 공격 Ability는 이 값이 있어야 정상 동작합니다.
	 *
	 * 사용 위치:
	 * - GetMeleeAttackData()
	 * - AttackMontage 재생
	 * - 소켓 기반 Trace 데이터 참조
	 */
	UPROPERTY(Transient)
	TObjectPtr<UFTMeleeDataAsset> ActiveMeleeData = nullptr;

	/**
	 * 이번 공격 판정 중 이미 맞은 Actor 목록입니다.
	 *
	 * 목적:
	 * - NotifyTick에서 매 프레임 오버랩을 수행하므로 같은 대상이 여러 번 감지될 수 있습니다.
	 * - 한 번의 공격에서 같은 대상에게 효과가 여러 번 들어가는 것을 막기 위해 사용합니다.
	 *
	 * TWeakObjectPtr을 쓰는 이유:
	 * - 타격 대상 Actor가 도중에 Destroy되어도 안전하게 무효화될 수 있습니다.
	 */
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<AActor>> HitActors;

	/**
	 * 현재 근접 판정이 활성화되어 있는지 나타냅니다.
	 *
	 * true:
	 * - PerformMeleeTrace가 실제 오버랩 판정을 수행합니다.
	 *
	 * false:
	 * - PerformMeleeTrace가 바로 return합니다.
	 *
	 * 변경 위치:
	 * - StartMeleeTrace()에서 true
	 * - StopMeleeTrace()에서 false
	 */
	bool bTraceActive = false;
};