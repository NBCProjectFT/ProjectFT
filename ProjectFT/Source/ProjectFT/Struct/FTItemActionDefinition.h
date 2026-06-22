#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FTItemActionDefinition.generated.h"

class UFTGA_UseItem;
class AFTProjectileActor;
class UGameplayEffect;
class UAnimMontage;

/** One usable behavior exposed by an item. General use and weapon attacks share this data. */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTItemActionDefinition
{
	GENERATED_BODY()

	/** Input that invokes this action. Empty means the primary item input. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Tags")
	FGameplayTag InputTag;

	/** Stable identifier used to find the granted ability instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Tags")
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|GAS")
	TSubclassOf<UFTGA_UseItem> AbilityClass;

	/** Applied to self by general item abilities or to the hit target by weapon abilities. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|GAS")
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|GAS")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Timing", meta = (ClampMin = "0.0"))
	float CastTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Timing", meta = (ClampMin = "0.0"))
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action")
	bool bConsumeOnUse = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat", meta = (ClampMin = "0.0"))
	float Damage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat", meta = (ClampMin = "0.0"))
	float Range = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat", meta = (ClampMin = "0.01"))
	float HitWindowDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat|Projectile")
	TSubclassOf<AFTProjectileActor> ProjectileClass;
};
