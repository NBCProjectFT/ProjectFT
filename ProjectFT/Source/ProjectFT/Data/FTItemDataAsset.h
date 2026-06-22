#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "../Struct/FTItemDataStruct.h"
#include "FTItemDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTItemDataAsset : public UPrimaryDataAsset
{	
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FTItemDataStruct ItemData;

	/** Multiple capabilities such as Item.Action.Heal and Item.Action.Throw. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Tags")
	FGameplayTagContainer ItemTags;

	/** The behavior selected by the primary-use input for this item. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Tags")
	FGameplayTag PrimaryUseTag;

	/** GAS ability granted while this item is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|GAS")
	TSubclassOf<class UGameplayAbility> UseAbilityClass;

	/** Effect applied by the generic item-use ability. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|GAS")
	TSubclassOf<class UGameplayEffect> UseEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|GAS", meta = (ClampMin = "0.0"))
	float UseCastTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|GAS", meta = (ClampMin = "0.0"))
	float UseCooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|GAS")
	bool bConsumeOnUse = true;

	/** Montage played by the granted item or weapon Gameplay Ability. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|GAS")
	TSoftObjectPtr<class UAnimMontage> UseMontage;

	/** Optional weapon definition. When set, this item grants every weapon action ability. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Weapon")
	TObjectPtr<class UFTWeaponDataAsset> WeaponDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Equip")
	FName EquipSocketName = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Equip")
	FTransform EquipRelativeTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Weapon|Melee",
		meta = (ClampMin = "0.1"))
	float MeleeHitBoundsScale = 1.0f;
};
