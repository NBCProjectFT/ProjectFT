#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_UseItem.h"
#include "ProjectFT/Struct/FTItemActionDefinition.h"
#include "FTWeaponGameplayAbility.generated.h"

class AFTItemActor;
class UAbilitySystemComponent;

/** GAS weapon action using the same item activation pipeline as every other item. */
UCLASS(Abstract)
class PROJECTFT_API UFTWeaponGameplayAbility : public UFTGA_UseItem
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
	virtual bool PrepareItemUse() override;
	virtual bool ExecuteItemUse() override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool ExecuteWeaponAction() PURE_VIRTUAL(
		UFTWeaponGameplayAbility::ExecuteWeaponAction, return false;);
	virtual bool ShouldEndImmediately() const override { return true; }

	AFTItemActor* GetItemActor() const;
	const FFTItemActionDefinition* GetActionDefinition() const;
	FTransform GetItemSocketTransform(FName SocketName) const;
	FTransform GetMuzzleTransform() const;
	bool ApplyWeaponGameplayEffect(AActor* TargetActor) const;
	static UAbilitySystemComponent* ResolveAbilitySystemComponent(AActor* TargetActor);
	void FinishAbility(bool bWasCancelled = false) { FinishItemUse(bWasCancelled); }

	/**
	 * Temporary SetByCaller magnitude for native FTGE_Damage.
	 * Specific weapon Blueprint abilities/effects can override this or use an EffectClass with fixed values.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Weapon|GAS",
		meta = (ClampMin = "0.0"))
	float SetByCallerDamage = 20.0f;

	UPROPERTY(Transient)
	TObjectPtr<AFTItemActor> ActiveItem;

	const FFTItemActionDefinition* ActiveDefinition = nullptr;
};
