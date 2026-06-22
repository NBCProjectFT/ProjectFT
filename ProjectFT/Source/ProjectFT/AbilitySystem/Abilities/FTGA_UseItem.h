#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "FTGA_UseItem.generated.h"

class UGameplayEffect;
class UFTItemDataAsset;
struct FFTItemActionDefinition;

/** Common activation pipeline for every usable item, including weapons. */
UCLASS()
class PROJECTFT_API UFTGA_UseItem : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_UseItem();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Resolve runtime references before montage/cast execution. */
	virtual bool PrepareItemUse() { return true; }

	/** General items apply their configured effect. Weapon abilities override this. */
	virtual bool ExecuteItemUse();
	virtual bool ShouldEndImmediately() const { return true; }
	virtual void OnItemConsumed();

	const UFTItemDataAsset* GetItemData() const;
	const FFTItemActionDefinition* GetItemActionDefinition() const;
	float GetUseCastTime() const;
	float GetUseCooldown() const;
	TSubclassOf<UGameplayEffect> GetUseEffectClass() const;
	bool PlayItemMontage();
	void FinishItemUse(bool bWasCancelled = false);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Item")
	TSubclassOf<UGameplayEffect> ItemEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Item", meta = (ClampMin = "0.0"))
	float CastTimeSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Item", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 0.0f;

	float PlayedMontageDuration = 0.0f;

private:
	void PerformItemUse();

	UFUNCTION()
	void OnCastFinished();
};
