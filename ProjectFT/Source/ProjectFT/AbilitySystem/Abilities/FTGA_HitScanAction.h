
#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_HitScanAction.generated.h"

class UFTItemDataAsset;
class UFTHitScanDataAsset;
class UMeshComponent;
class UAbilityTask_PlayMontageAndWait;
struct FFTHitScanActionStruct;

/**
 * 즉발 판정 무기용 어빌리티다.
 *
 * 입력 흐름:
 * - Event.UseItem으로 발동한다.
 * - HitScan DataAsset에서 사거리/소켓/몽타주 정보를 읽는다.
 * - 카메라 방향 또는 무기 소켓 기준으로 LineTrace를 수행한다.
 * - 맞은 대상에게 ItemData.UseData.UseEffects를 적용한다.
 */
UCLASS()
class PROJECTFT_API UFTGA_HitScanAction : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_HitScanAction();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
		) override;

private:
	// 실제 LineTrace를 수행하고, 맞은 대상에게 UseEffects를 적용한다.
	void PerformHitScan();

	// 히트스캔 어빌리티 종료 공통 처리와 활성 데이터 캐시 정리.
	void EndHitScanAbility(bool bWasCancelled);

	// 현재 활성화된 HitScan DataAsset에서 공격 데이터만 꺼낸다.
	const FFTHitScanActionStruct* GetHitScanActionData() const;

	// 캐릭터 또는 장착 액터들 중 RequiredSocketName을 가진 Mesh를 찾는다.
	UMeshComponent* ResolveWeaponMesh(AActor* Avatar, FName RequiredSocketName) const;

	// 몽타주가 정상 종료되면 어빌리티를 정상 종료한다.
	UFUNCTION()
	void HandleMontageCompleted();

	// 몽타주가 끊기거나 취소되면 어빌리티도 취소로 종료한다.
	UFUNCTION()
	void HandleMontageInterrupted();
	
	// 이번 활성에서 사용 중인 원본 아이템 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;
	
	// ActiveItemData를 히트스캔 데이터로 캐스팅한 캐시.
	UPROPERTY(Transient)
	TObjectPtr<UFTHitScanDataAsset> ActiveHitScanData = nullptr;
};
