#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTEquipmentComponent.generated.h"

class AFTWeaponActor;
class USceneComponent;

UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTEquipmentComponent : public UActorComponent
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

	UFUNCTION(BlueprintPure, Category = "FT|Equipment")
	AFTWeaponActor* GetEquippedWeapon() const { return EquippedWeapon; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Equipment")
	TSubclassOf<AFTWeaponActor> StartingWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Equipment")
	FName WeaponSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Equipment")
	FTransform WeaponRelativeTransform;

private:
	USceneComponent* ResolveAttachTarget() const;
	FName ResolveWeaponSocketName(USceneComponent* AttachTarget) const;

	UPROPERTY(Transient)
	TObjectPtr<AFTWeaponActor> EquippedWeapon;
};
