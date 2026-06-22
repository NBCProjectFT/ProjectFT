#include "FTItemActor.h"

#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"


AFTItemActor::AFTItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsBody"));
}

void AFTItemActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateAppearance();
}

bool AFTItemActor::Interact_Implementation(AActor* Interactor)
{
	if (!ItemData ) return false;
	
	// 해야할 일: 메시지 발송
	
	UE_LOG(LogFTItem, Log, TEXT("Picked up item: %s"), *ItemData->ItemData.ItemName.ToString());
	
	DestroyItem();
	return true;
}

void AFTItemActor::UpdateAppearance()
{
	if (ItemData && !ItemData->ItemData.ItemMesh.IsNull())
	{
		// 아이템 드랍 시점에 Mesh 로드(1회)
		UStaticMesh* LoadedMesh = ItemData->ItemData.ItemMesh.LoadSynchronous();
		if (LoadedMesh)
		{
			MeshComponent->SetStaticMesh(LoadedMesh);
		}
	}
}

void AFTItemActor::DestroyItem()
{
	Destroy();
}