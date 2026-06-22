#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Weapon/Ability/FTWeaponGameplayAbility.h"
#include "FTMeleeWeaponGameplayAbility.generated.h"

UCLASS()
class PROJECTFT_API UFTMeleeWeaponGameplayAbility : public UFTWeaponGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void NotifyWindowBegin() override;
	virtual void NotifyWindowTick() override;
	virtual void NotifyWindowEnd() override;

protected:
	virtual bool ExecuteWeaponAction() override;
	virtual bool ShouldEndImmediately() const override { return false; }
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void CheckMeleeHits();
	void HandleSafetyTimeout();

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> ActiveHitComponent;

	TSet<TWeakObjectPtr<AActor>> HitActors;
	FTimerHandle SafetyEndTimer;
};
