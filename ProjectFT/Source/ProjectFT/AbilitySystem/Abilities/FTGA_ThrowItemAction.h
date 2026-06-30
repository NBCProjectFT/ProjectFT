#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_ThrowItemAction.generated.h"

class UFTItemDataAsset;
class UFTThrowDataAsset;
class UFTProjectileActorDataAsset;
class AFTProjectileActor;
class UMeshComponent;
class UAnimMontage;

struct FFTThrowActorStruct;
struct FFTProjectileActorStruct;
struct FGameplayEventData;

UCLASS()
class PROJECTFT_API UFTGA_ThrowItemAction : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_ThrowItemAction();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

private:
	bool StartHoldingProjectile();
	bool ReleaseHeldProjectile();

	bool EnsureProjectileAbilityGranted();

	void EndThrowAbility(bool bWasCancelled);

	bool WaitForUseReleased();
	bool WaitForThrowRelease();

	UFUNCTION()
	void HandleUseReleasedEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleThrowReleaseEvent(FGameplayEventData Payload);

	void HandleThrowMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void ClearHeldProjectile();
	
	const FFTThrowActorStruct* GetThrowActorData() const;
	const FFTProjectileActorStruct* GetProjectileActorData() const;
	
	UMeshComponent* ResolveAttachMesh(
		AActor* Avatar,
		FName RequiredSocketName
	) const;

	FVector GetViewDirection() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UFTThrowDataAsset> ActiveThrowData = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileActorData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AFTProjectileActor> HeldProjectile = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveThrowMontage = nullptr;
	
	UPROPERTY(Transient)
	bool bIsHoldingProjectile = false;

	UPROPERTY(Transient)
	bool bWaitingForThrowRelease = false;
};
