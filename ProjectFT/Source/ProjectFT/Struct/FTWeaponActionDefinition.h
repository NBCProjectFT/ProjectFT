#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FTWeaponActionDefinition.generated.h"

class UFTWeaponAction;
class UFTWeaponGameplayAbility;
class UAnimMontage;
class AFTProjectileActor;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTWeaponActionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon|Legacy",
		meta = (DeprecatedProperty, DeprecationMessage = "Use AbilityClass. Legacy action objects are no longer executed."))
	TSubclassOf<UFTWeaponAction> ActionClass;

	/** GAS ability granted while this weapon is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon|GAS")
	TSubclassOf<UFTWeaponGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon", meta = (ClampMin = "0.0"))
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon", meta = (ClampMin = "0.0"))
	float Range = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon", meta = (ClampMin = "0.0"))
	float Cooldown = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	TSoftObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon", meta = (ClampMin = "0.01"))
	float HitWindowDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon|Projectile")
	TSubclassOf<AFTProjectileActor> ProjectileClass;

	/** Optional GAS damage effect. Damage is supplied with Data.Damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
