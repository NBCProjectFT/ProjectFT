#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Damage.generated.h"

/** Instant Health modifier. Data.Damage must be supplied as a negative magnitude. */
UCLASS()
class PROJECTFT_API UFTGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Damage();
};
