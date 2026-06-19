#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Struct/FTWeaponActionDefinition.h"
#include "FTWeaponActionComponent.generated.h"

class UFTWeaponAction;
class UFTWeaponDataAsset;


UCLASS(ClassGroup = (FT))
class PROJECTFT_API UFTWeaponActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTWeaponActionComponent();

	bool InitializeActions(const UFTWeaponDataAsset* WeaponDataAsset);

	bool StartAction(FGameplayTag ActionTag);

private:
	bool AddAction(const FFTWeaponActionDefinition& Definition);

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UFTWeaponAction>> Actions;
};
