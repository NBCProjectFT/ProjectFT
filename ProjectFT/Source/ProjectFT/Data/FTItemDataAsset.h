#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "../Struct/FTItemDataStruct.h"
#include "ProjectFT/Struct/FTItemActionDefinition.h"
#include "FTItemDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTItemDataAsset : public UPrimaryDataAsset
{	
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FTItemDataStruct ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Visual",
		meta = (ClampMin = "0.01"))
	FVector ItemMeshScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Projectile")
	bool bSpawnItemOnProjectileImpact = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Projectile",
		meta = (ClampMin = "0.01", DisplayName = "Projectile Collision Scale"))
	float ProjectileCollisionScale = 1.0f;

	/** Multiple capabilities such as Item.Action.Heal and Item.Action.Throw. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Tags")
	FGameplayTagContainer ItemTags;

	/** Every behavior this item can perform, including healing, throwing and attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Actions")
	TArray<FFTItemActionDefinition> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Equip")
	FName EquipSocketName = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Equip")
	FTransform EquipRelativeTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Attack|RangeAtatck")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Socket pair used by melee abilities. Sweeping a sphere between them creates the attack capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Attack|MeleeAttack")
	FName MeleeTraceStartSocketName = TEXT("Hit_Start");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Attack|MeleeAttack")
	FName MeleeTraceEndSocketName = TEXT("Hit_End");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Attack|MeleeAttack",
		meta = (ClampMin = "1.0"))
	float MeleeTraceRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Action|MeleeAttack")
	bool bDrawMeleeTraceDebug = false;

};
