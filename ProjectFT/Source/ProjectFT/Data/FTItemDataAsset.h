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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Action")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data|Action|Melee",
		meta = (ClampMin = "0.1"))
	float MeleeHitBoundsScale = 1.0f;
};
