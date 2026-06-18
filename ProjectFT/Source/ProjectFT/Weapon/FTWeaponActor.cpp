#include "FTWeaponActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProjectFT/Components/FTWeaponManagerComponent.h"

AFTWeaponActor::AFTWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(SceneRoot);

	AttackComponentManager = CreateDefaultSubobject<UFTWeaponManagerComponent>(TEXT("AttackComponentManager"));
}

void AFTWeaponActor::Attack()
{
	AttackComponentManager->Attack();
}

bool AFTWeaponActor::Equip(FName ItemName)
{
	if (!AttackComponentManager->SetWeaponData(ItemName))
	{
		return false;
	}

	EquippedItemName = ItemName;
	return true;
}

void AFTWeaponActor::UnEquip()
{
	AttackComponentManager->ClearWeaponData();
	EquippedItemName = NAME_None;
}
