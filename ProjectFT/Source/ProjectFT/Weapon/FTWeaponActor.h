#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FTWeaponActor.generated.h"

class UFTWeaponManagerComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECTFT_API AFTWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AFTWeaponActor();

	UFUNCTION(BlueprintCallable, Category = "FT|Weapon")
	void Attack();

	UFUNCTION(BlueprintCallable, Category = "FT|Weapon")
	bool Equip(FName ItemName);

	UFUNCTION(BlueprintCallable, Category = "FT|Weapon")
	void UnEquip();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	TObjectPtr<UFTWeaponManagerComponent> AttackComponentManager;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FT|Weapon")
	FName EquippedItemName = NAME_None;
};
