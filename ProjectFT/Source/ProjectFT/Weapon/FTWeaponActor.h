#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "../Item/FTItemActor.h"
#include "FTWeaponActor.generated.h"

class UFTWeaponDataAsset;
class UCapsuleComponent;
struct FFTWeaponActionDefinition;

UCLASS(Blueprintable)
class PROJECTFT_API AFTWeaponActor : public AFTItemActor
{
	GENERATED_BODY()

public:
	AFTWeaponActor();

	const UFTWeaponDataAsset* GetWeaponDataAsset() const { return WeaponDataAsset; }
	const FFTWeaponActionDefinition* FindActionDefinition(FGameplayTag ActionTag) const;

	FTransform GetWeaponMuzzleTransform() const;
	UCapsuleComponent* GetMeleeHitComponent() const { return MeleeHitCapsule; }

protected:
	virtual void BeginPlay() override;
	void ConfigureMeleeHitCapsule();
	void FitMeleeHitCapsuleToMesh();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Weapon")
	TObjectPtr<UFTWeaponDataAsset> WeaponDataAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	TObjectPtr<UCapsuleComponent> MeleeHitCapsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Weapon|Melee", meta = (ClampMin = "0.1"))
	float MeleeHitBoundsScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");
};
