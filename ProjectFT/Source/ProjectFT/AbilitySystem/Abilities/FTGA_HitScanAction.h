
#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_HitScanAction.generated.h"

class UFTItemDataAsset;
class UFTHitScanDataAsset;
class UMeshComponent;
class UAbilityTask_PlayMontageAndWait;
struct FFTHitScanActionStruct;

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
	void PerformHitScan();

	void EndHitScanAbility(bool bWasCancelled);

	const FFTHitScanActionStruct* GetHitScanActionData() const;

	UMeshComponent* ResolveWeaponMesh(AActor* Avatar, FName RequiredSocketName) const;

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();
	
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UFTHitScanDataAsset> ActiveHitScanData = nullptr;
};
