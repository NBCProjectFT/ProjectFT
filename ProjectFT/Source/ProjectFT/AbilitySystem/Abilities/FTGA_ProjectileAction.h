
#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_ProjectileAction.generated.h"

class UFTItemDataAsset;
class UFTProjectileActorDataAsset;

UCLASS()
class PROJECTFT_API UFTGA_ProjectileAction : public UFTGA_ItemAbility
{
	GENERATED_BODY()
	
public:
	UFTGA_ProjectileAction();
	virtual void ActivateAbility
	(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData
		) override;
	
private:
	
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileActorData = nullptr;
};
