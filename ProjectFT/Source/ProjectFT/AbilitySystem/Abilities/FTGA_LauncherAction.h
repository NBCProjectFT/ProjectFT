#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_LauncherAction.generated.h"

class UFTItemDataAsset;
class UFTLauncherDataAsset;
class UFTProjectileActorDataAsset;
class UMeshComponent;

struct FFTLauncherActionStruct;
struct FFTProjectileActorStruct;

UCLASS()
class PROJECTFT_API UFTGA_LauncherAction : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_LauncherAction();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

private:
	bool FireProjectile();

	bool EnsureProjectileAbilityGranted();

	void EndLauncherAbility(bool bWasCancelled);

	const FFTLauncherActionStruct* GetLauncherActionData() const;
	const FFTProjectileActorStruct* GetProjectileActorData() const;

	UMeshComponent* ResolveLauncherMesh(
		AActor* Avatar,
		FName RequiredSocketName
	) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTLauncherDataAsset> ActiveLauncherData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileActorData = nullptr;
};