#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Weapon/Action/FTWeaponAction.h"
#include "FTMeleeWeaponAction.generated.h"

UCLASS()
class PROJECTFT_API UFTMeleeWeaponAction : public UFTWeaponAction
{
	GENERATED_BODY()

public:
	virtual void NotifyWindowBegin() override;
	virtual void NotifyWindowTick() override;
	virtual void NotifyWindowEnd() override;

protected:
	virtual bool ExecuteAction() override;
	void CheckMeleeHits();
	void ProcessOverlappingActor(AActor* OtherActor);

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> ActiveHitComponent;

	TSet<TWeakObjectPtr<AActor>> HitActors;
};
