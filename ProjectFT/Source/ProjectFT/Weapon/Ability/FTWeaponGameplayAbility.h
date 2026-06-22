#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ProjectFT/Struct/FTWeaponActionDefinition.h"
#include "FTWeaponGameplayAbility.generated.h"

class AFTWeaponActor;

/** Base GAS action granted by FTQuickSlotComponent while a weapon item is equipped. */
UCLASS(Abstract)
class PROJECTFT_API UFTWeaponGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFTWeaponGameplayAbility();

	virtual void NotifyWindowBegin() {}
	virtual void NotifyWindowTick() {}
	virtual void NotifyWindowEnd() {}

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool ExecuteWeaponAction() PURE_VIRTUAL(
		UFTWeaponGameplayAbility::ExecuteWeaponAction, return false;);
	virtual bool ShouldEndImmediately() const { return true; }

	AFTWeaponActor* GetWeaponActor() const;
	const FFTWeaponActionDefinition* GetActionDefinition() const;
	bool PlayAttackMontage();
	bool ApplyWeaponDamage(AActor* TargetActor, float Damage) const;
	void FinishAbility(bool bWasCancelled = false);

	UPROPERTY(Transient)
	TObjectPtr<AFTWeaponActor> ActiveWeapon;

	const FFTWeaponActionDefinition* ActiveDefinition = nullptr;
	float PlayedMontageDuration = 0.0f;

private:
	mutable double LastExecutionTime = -DBL_MAX;
};
