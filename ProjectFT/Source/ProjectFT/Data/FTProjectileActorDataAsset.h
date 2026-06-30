
#pragma once

#include "CoreMinimal.h"
#include "FTItemDataAsset.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Struct/FTProjectileActorStruct.h"
#include "FTProjectileActorDataAsset.generated.h"

UCLASS(Blueprintable)
class PROJECTFT_API UFTProjectileActorDataAsset : public UFTItemDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Data")
	FFTProjectileActorStruct ProjectileActorData;
};
