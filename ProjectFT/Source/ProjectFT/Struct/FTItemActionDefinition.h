#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FTItemActionDefinition.generated.h"

class AFTProjectileActor;
class UGameplayEffect;
class UAnimMontage;

/** One usable behavior exposed by an item. General use and weapon attacks share this data. */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTItemActionDefinition
{
	GENERATED_BODY()

	/** Input that invokes this action. Empty means the primary item input. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Tags",
		meta = (Categories = "Input"))
	FGameplayTag InputTag;

	/** Stable identifier used to find the granted ability instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Tags",
		meta = (Categories = "Item.Action,Weapon.Action"))
	FGameplayTag ActionTag;

	/** Applied to self by general item abilities or to the hit target by weapon abilities. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|GAS")
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|GAS")
	TSoftObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action")
	bool bConsumeOnUse = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat", meta = (ClampMin = "0.0"))
	float Range = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat|Projectile")
	TSubclassOf<AFTProjectileActor> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat|Projectile",
		meta = (ClampMin = "1.0"))
	float ProjectileSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat|Projectile",
		meta = (ClampMin = "0.1"))
	float ProjectileLifeSpan = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat|Projectile",
		meta = (ClampMin = "0.0"))
	float ProjectileGravityScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Action|Combat|Projectile",
		meta = (ClampMin = "1.0"))
	float ProjectileCollisionRadius = 8.0f;
};
