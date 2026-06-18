#include "FTWeaponManagerComponent.h"

#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTWeaponStruct.h"
#include "ProjectFT/Components/FTAttackComponent.h"
#include "ProjectFT/Components/FTHitScanAttackComponent.h"

UFTWeaponManagerComponent::UFTWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFTWeaponManagerComponent::Attack()
{
	if (ActiveAttackComponent)
	{
		ActiveAttackComponent->Attack();
	}
}

bool UFTWeaponManagerComponent::SetWeaponData(FName ItemName)
{
	if (!WeaponDataTable || ItemName.IsNone())
	{
		return false;
	}

	const FFTWeaponStruct* WeaponRow = WeaponDataTable->FindRow<FFTWeaponStruct>(
		ItemName, TEXT("UFTAttackComponentManager::SetWeaponData"));
	if (!WeaponRow)
	{
		return false;
	}

	EquippedItemName = ItemName;
	if (!BuildAttackComponent())
	{
		EquippedItemName = NAME_None;
		return false;
	}

	return true;
}

void UFTWeaponManagerComponent::ClearWeaponData()
{
	ClearAttackComponent();
	EquippedItemName = NAME_None;
}

bool UFTWeaponManagerComponent::BuildAttackComponent()
{
	ClearAttackComponent();
	if (!WeaponDataTable || EquippedItemName.IsNone() || !GetOwner())
	{
		return false;
	}

	const FFTWeaponStruct* WeaponRow = WeaponDataTable->FindRow<FFTWeaponStruct>(
		EquippedItemName, TEXT("BuildAttackComponent"));
	if (!WeaponRow)
	{
		return false;
	}

	TSubclassOf<UFTAttackComponent> AttackComponentClass;
	if (WeaponRow->AttackTypeTag == TAG_FT_Weapon_Attack_Melee)
	{
		// TODO : 근접공격 컴포넌트 구현
	}
	else if (WeaponRow->AttackTypeTag == TAG_FT_Weapon_Attack_HitScan)
	{
		AttackComponentClass = UFTHitScanAttackComponent::StaticClass();
	}
	else if (WeaponRow->AttackTypeTag == TAG_FT_Weapon_Attack_Projectile)
	{
		// TODO : 투사체 공격 컴포넌트 구현
	}

	if (!AttackComponentClass)
	{
		return false;
	}

	ActiveAttackComponent = NewObject<UFTAttackComponent>(GetOwner(), AttackComponentClass);
	GetOwner()->AddInstanceComponent(ActiveAttackComponent);
	ActiveAttackComponent->RegisterComponent();
	return true;
}

void UFTWeaponManagerComponent::ClearAttackComponent()
{
	if (ActiveAttackComponent)
	{
		ActiveAttackComponent->DestroyComponent();
		ActiveAttackComponent = nullptr;
	}
}
