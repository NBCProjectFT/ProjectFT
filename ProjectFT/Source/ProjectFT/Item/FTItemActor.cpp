#include "FTItemActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTWeaponDataAsset.h"

AFTItemActor::AFTItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsBody"));

	MeleeHitCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("MeleeHitCapsule"));
	MeleeHitCapsule->SetupAttachment(MeshComponent);
	MeleeHitCapsule->InitCapsuleSize(18.0f, 55.0f);
	MeleeHitCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFTItemActor::BeginPlay()
{
	Super::BeginPlay();
	ConfigureFromItemData();
}

void AFTItemActor::InitializeFromItemData(UFTItemDataAsset* InItemData)
{
	ItemData = InItemData;
	ConfigureFromItemData();
}

void AFTItemActor::ConfigureFromItemData()
{
	UpdateAppearance();
	ConfigureMeleeHitCapsule();
}

bool AFTItemActor::Interact_Implementation(AActor* Interactor)
{
	if (!ItemData)
	{
		return false;
	}

	UE_LOG(LogFTItem, Log, TEXT("Picked up item: %s"), *ItemData->ItemData.ItemName.ToString());
	DestroyItem();
	return true;
}

void AFTItemActor::UpdateAppearance()
{
	if (!ItemData || ItemData->ItemData.ItemMesh.IsNull())
	{
		return;
	}

	if (UStaticMesh* LoadedMesh = ItemData->ItemData.ItemMesh.LoadSynchronous())
	{
		MeshComponent->SetStaticMesh(LoadedMesh);
	}
}

const UFTWeaponDataAsset* AFTItemActor::GetWeaponDataAsset() const
{
	return ItemData ? ItemData->WeaponDataAsset : nullptr;
}

const FFTWeaponActionDefinition* AFTItemActor::FindActionDefinition(FGameplayTag ActionTag) const
{
	const UFTWeaponDataAsset* WeaponData = GetWeaponDataAsset();
	if (!WeaponData || !ActionTag.IsValid())
	{
		return nullptr;
	}

	return WeaponData->Actions.FindByPredicate(
		[ActionTag](const FFTWeaponActionDefinition& Definition)
		{
			return Definition.ActionTag.MatchesTagExact(ActionTag);
		});
}

FTransform AFTItemActor::GetMuzzleTransform() const
{
	if (ItemData && MeshComponent && MeshComponent->DoesSocketExist(ItemData->MuzzleSocketName))
	{
		return MeshComponent->GetSocketTransform(ItemData->MuzzleSocketName, RTS_World);
	}
	return GetActorTransform();
}

void AFTItemActor::ConfigureMeleeHitCapsule()
{
	FitMeleeHitCapsuleToMesh();
	MeleeHitCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeleeHitCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeleeHitCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MeleeHitCapsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	MeleeHitCapsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	MeleeHitCapsule->SetGenerateOverlapEvents(true);
}

void AFTItemActor::FitMeleeHitCapsuleToMesh()
{
	if (!ItemData || !MeshComponent || !MeshComponent->GetStaticMesh() || !MeleeHitCapsule)
	{
		return;
	}

	FVector BoundsMin;
	FVector BoundsMax;
	MeshComponent->GetLocalBounds(BoundsMin, BoundsMax);
	const FVector Center = (BoundsMin + BoundsMax) * 0.5f;
	const FVector Extent = (BoundsMax - BoundsMin) * 0.5f * ItemData->MeleeHitBoundsScale;

	FVector CapsuleAxis = FVector::UpVector;
	float HalfHeight = Extent.Z;
	float Radius = FMath::Max(Extent.X, Extent.Y);
	if (Extent.X >= Extent.Y && Extent.X >= Extent.Z)
	{
		CapsuleAxis = FVector::ForwardVector;
		HalfHeight = Extent.X;
		Radius = FMath::Max(Extent.Y, Extent.Z);
	}
	else if (Extent.Y >= Extent.X && Extent.Y >= Extent.Z)
	{
		CapsuleAxis = FVector::RightVector;
		HalfHeight = Extent.Y;
		Radius = FMath::Max(Extent.X, Extent.Z);
	}

	Radius = FMath::Max(1.0f, Radius);
	HalfHeight = FMath::Max(Radius, HalfHeight);
	MeleeHitCapsule->SetRelativeLocation(Center);
	MeleeHitCapsule->SetRelativeRotation(
		FQuat::FindBetweenNormals(FVector::UpVector, CapsuleAxis));
	MeleeHitCapsule->SetCapsuleSize(Radius, HalfHeight);
}

void AFTItemActor::DestroyItem()
{
	Destroy();
}
