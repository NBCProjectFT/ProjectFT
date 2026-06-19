#include "FTWeaponActor.h"

#include "ProjectFT/Components/FTWeaponActionComponent.h"
#include "ProjectFT/Data/FTWeaponDataAsset.h"
#include "ProjectFT/Message/FTGameplayTags.h"

AFTWeaponActor::AFTWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ActionComponent = CreateDefaultSubobject<UFTWeaponActionComponent>(TEXT("ActionComponent"));
}

void AFTWeaponActor::BeginPlay()
{
	Super::BeginPlay();
	if (!WeaponDataAsset || !ActionComponent->InitializeActions(WeaponDataAsset))
	{
		UE_LOG(LogTemp, Error, TEXT("%s has no valid WeaponDataAsset."), *GetName());
	}
}

void AFTWeaponActor::Attack()
{
	ActionComponent->StartAction(TAG_FT_Weapon_Action_Primary);
}

FTransform AFTWeaponActor::GetWeaponMuzzleTransform() const
{
	if (MeshComponent && MeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return MeshComponent->GetSocketTransform(MuzzleSocketName, RTS_World);
	}

	return GetActorTransform();
}
