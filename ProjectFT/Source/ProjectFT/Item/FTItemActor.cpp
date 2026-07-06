#include "FTItemActor.h"

#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "FTItemPoolSubsystem.h"


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
	if (!ItemData) return false;
	
	// GameplayMessageSubsystem을 통해 아이템 획득 메시지 전송
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	Payload.ItemId = ItemData->ItemData.ItemId;
	Payload.InstigatorActor = Interactor;
	Payload.TargetActor = this;
	Payload.Value = 1.0f;

	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ItemPickedUp, Payload);
	
	UE_LOG(LogFTItem, Log, TEXT("%s 아이템 획득"), *ItemData->ItemData.ItemName.ToString());
	
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
	if (UWorld* World = GetWorld())
	{
		if (UFTItemPoolSubsystem* PoolSubsystem = World->GetSubsystem<UFTItemPoolSubsystem>())
		{
			// 파괴 권한을 서브시스템으로 양도 (풀링 반환 처리)
			PoolSubsystem->ReleaseItemActor(this);
			return;
		}
	}
	
	// 서브시스템이 없거나 오류 상황일 때만 직접 파괴 실행
	Destroy();
}