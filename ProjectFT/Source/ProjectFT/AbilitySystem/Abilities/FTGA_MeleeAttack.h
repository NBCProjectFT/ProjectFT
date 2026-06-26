#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_MeleeAttack.generated.h"

class UFTItemDataAsset;
class UFTMeleeDataAsset;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class USkeletalMeshComponent;
struct FFTMeleeAttackStruct;

UCLASS()
class PROJECTFT_API UFTGA_MeleeAttack : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_MeleeAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

private:
	
	UFUNCTION()
	void HandleMeleeBeginEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMeleeHitEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMeleeEndEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	void EndMeleeAbility(bool bWasCancelled);

	const FFTMeleeAttackStruct* GetMeleeAttackData() const;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;

	bool bMeleeTraceActive = false;
	
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTMeleeDataAsset> ActiveMeleeData = nullptr;

	bool bTraceActive = false;
};