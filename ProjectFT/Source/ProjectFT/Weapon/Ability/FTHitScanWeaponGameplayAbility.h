#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Weapon/Ability/FTWeaponGameplayAbility.h"
#include "FTHitScanWeaponGameplayAbility.generated.h"

UCLASS()
class PROJECTFT_API UFTHitScanWeaponGameplayAbility : public UFTWeaponGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecuteWeaponAction() override;
};
