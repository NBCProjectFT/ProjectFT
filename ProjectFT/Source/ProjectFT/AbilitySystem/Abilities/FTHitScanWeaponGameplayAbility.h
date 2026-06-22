#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTWeaponGameplayAbility.h"
#include "FTHitScanWeaponGameplayAbility.generated.h"

UCLASS()
class PROJECTFT_API UFTHitScanWeaponGameplayAbility : public UFTWeaponGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecuteWeaponAction() override;
};
