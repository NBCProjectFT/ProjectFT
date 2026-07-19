#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "FTCashierCharacter.generated.h"

UCLASS()
class PROJECTFT_API AFTCashierCharacter : public AFTAICharacterBase
{
	GENERATED_BODY()

public:
	AFTCashierCharacter();

protected:
	virtual void OnDeath() override;
};
