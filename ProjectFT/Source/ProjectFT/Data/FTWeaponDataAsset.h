#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Struct/FTWeaponActionDefinition.h"
#include "FTWeaponDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Weapon")
	FName WeaponName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Weapon")
	FGameplayTagContainer WeaponTags;

	/** Runtime actions available to this weapon. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Weapon")
	TArray<FFTWeaponActionDefinition> Actions;
};
