#pragma once

#include "CoreMinimal.h"
#include "../Item/FTItemActor.h"
#include "ProjectFT/Interface/FTWeaponSource.h"
#include "FTWeaponActor.generated.h"

class UFTWeaponActionComponent;
class UFTWeaponDataAsset;

UCLASS(Blueprintable)
class PROJECTFT_API AFTWeaponActor : public AFTItemActor, public IFTWeaponSource
{
	GENERATED_BODY()

public:
	AFTWeaponActor();

	void Attack();

	virtual FTransform GetWeaponMuzzleTransform() const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Weapon")
	TObjectPtr<UFTWeaponDataAsset> WeaponDataAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	TObjectPtr<UFTWeaponActionComponent> ActionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");
};
