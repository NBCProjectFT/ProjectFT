#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Weapon/Action/FTWeaponAction.h"
#include "FTHitScanWeaponAction.generated.h"

UCLASS()
class PROJECTFT_API UFTHitScanWeaponAction : public UFTWeaponAction
{
	GENERATED_BODY()

protected:
	virtual bool ExecuteAction() override;
};
