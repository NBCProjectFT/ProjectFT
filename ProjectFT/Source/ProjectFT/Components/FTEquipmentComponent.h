#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "ProjectFT/Core/GameplayMessageProcessor.h"
#include "FTEquipmentComponent.generated.h"

class AFTWeaponActor;
class USceneComponent;
class UAbilitySystemComponent;
class UFTWeaponGameplayAbility;
class UFTWeaponDataAsset;
struct FFTMessagePayloadStruct;

UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTEquipmentComponent : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	UFTEquipmentComponent();

	UFUNCTION(BlueprintCallable, Category = "FT|Equipment")
	bool SpawnAndEquipWeapon(TSubclassOf<AFTWeaponActor> WeaponClass);

	UFUNCTION(BlueprintCallable, Category = "FT|Equipment")
	bool EquipWeapon(AFTWeaponActor* Weapon);

	UFUNCTION(BlueprintCallable, Category = "FT|Equipment")
	void UnequipWeapon();

	UFUNCTION(BlueprintCallable, Category = "FT|Equipment")
	void AttackPrimary();

	UFUNCTION(BlueprintCallable, Category = "FT|Equipment|GAS")
	bool TryActivateWeaponAbility(FGameplayTag ActionTag);

	void NotifyWeaponActionWindowBegin(FGameplayTag ActionTag);
	void NotifyWeaponActionWindowTick(FGameplayTag ActionTag);
	void NotifyWeaponActionWindowEnd(FGameplayTag ActionTag);

	UFUNCTION(BlueprintPure, Category = "FT|Equipment")
	AFTWeaponActor* GetEquippedWeapon() const { return EquippedWeapon; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void StartListening() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Equipment")
	TSubclassOf<AFTWeaponActor> StartingWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Equipment")
	FName WeaponSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Equipment")
	FTransform WeaponRelativeTransform;

private:
	void OnPrimaryAttackRequested(
		FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);

	USceneComponent* ResolveAttachTarget() const;
	FName ResolveWeaponSocketName(USceneComponent* AttachTarget) const;
	bool EnsureAbilitySystem();
	bool GrantWeaponAbilities(AFTWeaponActor* Weapon);
	void RemoveWeaponAbilities();
	TSubclassOf<UFTWeaponGameplayAbility> ResolveDefaultAbilityClass(
		const UFTWeaponDataAsset* WeaponData) const;
	FGameplayAbilitySpecHandle FindWeaponAbilityHandle(FGameplayTag ActionTag) const;
	UFTWeaponGameplayAbility* GetActiveWeaponAbility(FGameplayTag ActionTag) const;

	UPROPERTY(Replicated, Transient)
	TObjectPtr<AFTWeaponActor> EquippedWeapon;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	TMap<FGameplayTag, FGameplayAbilitySpecHandle> GrantedWeaponAbilities;
};
