#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/WeaponAbility/FTWeaponGameplayAbility.h"
#include "FTProjectileWeaponGameplayAbility.generated.h"

UCLASS()
class PROJECTFT_API UFTProjectileWeaponGameplayAbility : public UFTWeaponGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecuteWeaponAction() override;
};
