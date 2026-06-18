#pragma once

#include "CoreMinimal.h"
#include "FTAttackComponent.h"
#include "FTHitScanAttackComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = (FT))
class PROJECTFT_API UFTHitScanAttackComponent : public UFTAttackComponent
{
	GENERATED_BODY()

public:
	virtual void Attack() override;
};
