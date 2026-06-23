#include "FTItemActor.h"

#include "Components/StaticMeshComponent.h"
#include "ProjectFT/Components/FTProjectileComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

AFTItemActor::AFTItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsBody"));

	ProjectileComponent = CreateDefaultSubobject<UFTProjectileComponent>(TEXT("ProjectileComponent"));
}

void AFTItemActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateAppearance();
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

void AFTItemActor::InitializeFromItemData(UFTItemDataAsset* InItemData)
{
	ItemData = InItemData;
	UpdateAppearance();
}

void AFTItemActor::SetEquipped(bool bEquipped)
{
	if (!MeshComponent)
	{
		return;
	}

	if (bEquipped)
	{
		MeshComponent->SetSimulatePhysics(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	MeshComponent->SetCollisionProfileName(TEXT("PhysicsBody"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetSimulatePhysics(true);
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
		MeshComponent->SetRelativeScale3D(ItemData->ItemMeshScale);
	}
}

void AFTItemActor::DestroyItem()
{
	Destroy();
}
