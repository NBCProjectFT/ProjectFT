#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FTWeaponSource.generated.h"

UINTERFACE()
class PROJECTFT_API UFTWeaponSource : public UInterface
{
	GENERATED_BODY()
};

/** Stable contract used by weapon actions instead of depending on a concrete weapon class. */
class PROJECTFT_API IFTWeaponSource
{
	GENERATED_BODY()

public:
	virtual FTransform GetWeaponMuzzleTransform() const = 0;
};
