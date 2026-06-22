#include "FTWeaponActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProjectFT/Data/FTWeaponDataAsset.h"

AFTWeaponActor::AFTWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	MeleeHitCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("MeleeHitCapsule"));
	MeleeHitCapsule->SetupAttachment(MeshComponent);
	MeleeHitCapsule->InitCapsuleSize(18.0f, 55.0f);
	MeleeHitCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFTWeaponActor::BeginPlay()
{
	Super::BeginPlay();
	ConfigureMeleeHitCapsule();

	if (!WeaponDataAsset || WeaponDataAsset->Actions.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("%s has no valid WeaponDataAsset."), *GetName());
	}
}

void AFTWeaponActor::ConfigureMeleeHitCapsule()
{
	FitMeleeHitCapsuleToMesh();
	MeleeHitCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeleeHitCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeleeHitCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MeleeHitCapsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	MeleeHitCapsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	MeleeHitCapsule->SetGenerateOverlapEvents(true);
}

void AFTWeaponActor::FitMeleeHitCapsuleToMesh()
{
	if (!MeshComponent || !MeshComponent->GetStaticMesh() || !MeleeHitCapsule)
	{
		return;
	}

	FVector BoundsMin;
	FVector BoundsMax;
	MeshComponent->GetLocalBounds(BoundsMin, BoundsMax);

	const FVector Center = (BoundsMin + BoundsMax) * 0.5f;
	const FVector Extent = (BoundsMax - BoundsMin) * 0.5f * MeleeHitBoundsScale;

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

FTransform AFTWeaponActor::GetWeaponMuzzleTransform() const
{
	if (MeshComponent && MeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return MeshComponent->GetSocketTransform(MuzzleSocketName, RTS_World);
	}

	return GetActorTransform();
}

const FFTWeaponActionDefinition* AFTWeaponActor::FindActionDefinition(FGameplayTag ActionTag) const
{
	if (!WeaponDataAsset || !ActionTag.IsValid())
	{
		return nullptr;
	}

	return WeaponDataAsset->Actions.FindByPredicate(
		[ActionTag](const FFTWeaponActionDefinition& Definition)
		{
			return Definition.ActionTag.MatchesTagExact(ActionTag);
		});
}
