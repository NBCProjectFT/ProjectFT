#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FTWeaponActionDefinition.generated.h"

class UFTWeaponAction;
class UAnimMontage;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTWeaponActionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	TSubclassOf<UFTWeaponAction> ActionClass;

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
};
