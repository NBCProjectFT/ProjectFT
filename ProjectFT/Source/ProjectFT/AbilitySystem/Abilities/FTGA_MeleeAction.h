#pragma once

#include "CoreMinimal.h"
#include "FTGA_DamageItemAbility.h"
#include "FTGA_MeleeAction.generated.h"

class UFTItemDataAsset;
class UFTMeleeDataAsset;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class USkeletalMeshComponent;
struct FFTMeleeActionStruct;

/**
 * 근접 무기 아이템을 사용할 때 실행되는 어빌리티다.
 *
 * 입력 흐름:
 * - PlayerCharacter/EquipmentComponent가 아이템 DataAsset을 Payload.OptionalObject로 담아 Event.UseItem을 보낸다.
 * - 이 어빌리티가 UFTMeleeDataAsset을 캐싱하고 AttackMontage를 재생한다.
 * - 몽타주 안의 UFTMeleeActionTraceNotifyState가 Trace Begin/Hit/End 이벤트를 다시 보내준다.
 * - Hit 이벤트가 들어오면 맞은 액터에게 ItemData.UseData.UseEffects를 적용한다.
 */
UCLASS()
class PROJECTFT_API UFTGA_MeleeAction : public UFTGA_DamageItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_MeleeAction();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

private:
	// AnimNotifyState가 판정 구간 시작을 알릴 때 호출된다. 중복 타격 목록을 초기화한다.
	UFUNCTION()
	void HandleMeleeBeginEvent(FGameplayEventData Payload);

	// AnimNotifyState가 매 틱 Overlap으로 찾은 대상들을 전달할 때 호출된다.
	UFUNCTION()
	void HandleMeleeHitEvent(FGameplayEventData Payload);

	// AnimNotifyState가 판정 구간 종료를 알릴 때 호출된다.
	UFUNCTION()
	void HandleMeleeEndEvent(FGameplayEventData Payload);

	// 몽타주가 정상 종료되면 어빌리티를 정상 종료한다.
	UFUNCTION()
	void HandleMontageCompleted();

	// 몽타주가 끊기거나 취소되면 어빌리티도 취소로 종료한다.
	UFUNCTION()
	void HandleMontageInterrupted();

	// 근접 공격 어빌리티 종료 공통 처리.
	void EndMeleeAbility(bool bWasCancelled);

	// 현재 활성화된 Melee DataAsset에서 공격 데이터만 꺼낸다.
	const FFTMeleeActionStruct* GetMeleeActionData() const;

	// 한 번의 공격 구간에서 같은 액터를 여러 번 맞히지 않기 위한 목록.
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;

	// AnimNotifyState가 보낸 Trace Begin/End 상태.
	bool bMeleeTraceActive = false;
	
	// 이번 활성에서 사용 중인 원본 아이템 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;

	// ActiveItemData를 근접 무기 데이터로 캐스팅한 캐시.
	UPROPERTY(Transient)
	TObjectPtr<UFTMeleeDataAsset> ActiveMeleeData = nullptr;

	// TODO: bMeleeTraceActive와 역할이 겹친다. 후속 정리 때 하나로 합칠 수 있다.
	bool bTraceActive = false;
};
