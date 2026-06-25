
#pragma once

#include "CoreMinimal.h"
#include "FTGameplayAbility.h"
#include "FTGA_ProjectileAttack.generated.h"

class UFTItemDataAsset;
class UFTProjectileDataAsset;
class USkeletalMeshComponent;
struct FFTProjectileAttackStruct;

UCLASS()
class PROJECTFT_API UFTGA_ProjectileAttack : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_ProjectileAttack();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
		) override;

	void EndProjectileAbility(bool bWasCancelled);

	const FFTProjectileAttackStruct* GetProjectileAttackData() const;

	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ItemDataAsset = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTProjectileDataAsset> ProjectileDataAsset = nullptr;

};
