#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Weapon/Action/FTWeaponAction.h"
#include "FTProjectileWeaponAction.generated.h"

UCLASS()
class PROJECTFT_API UFTProjectileWeaponAction : public UFTWeaponAction
{
	GENERATED_BODY()

protected:
	virtual bool ExecuteAction() override;
};
