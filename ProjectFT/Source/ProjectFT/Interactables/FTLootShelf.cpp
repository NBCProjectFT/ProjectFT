// Fill out your copyright notice in the Description page of Project Settings.

#include "FTLootShelf.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

#include "ProjectFT/Components/FTChanneledInteractionComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"

#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

AFTLootShelf::AFTLootShelf()
{
	PrimaryActorTick.bCanEverTick = false;

	// 진열대 메시를 루트로. 테스트가 바로 되도록 엔진 기본 큐브를 할당한다(에디터에서 교체 가능).
	ShelfMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelfMesh"));
	SetRootComponent(ShelfMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		ShelfMesh->SetStaticMesh(CubeMesh.Object);
	}
	// StaticMesh는 기본적으로 Visibility 채널을 Block 하므로 상호작용 트레이스에 잡힌다.

	// 꾹 눌러 훔치는 채널형 상호작용 컴포넌트.
	ChanneledInteraction = CreateDefaultSubobject<UFTChanneledInteractionComponent>(TEXT("ChanneledInteraction"));
}

void AFTLootShelf::BeginPlay()
{
	Super::BeginPlay();

	// 게이지 완료 시 훔치기 성공 처리.
	if (ChanneledInteraction)
	{
		ChanneledInteraction->OnCompleted.AddDynamic(this, &AFTLootShelf::HandleStealCompleted);
	}
}

FText AFTLootShelf::GetInteractionPrompt_Implementation() const
{
	return InteractionPrompt;
}

float AFTLootShelf::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (bHasBeenLooted) return ActualDamage;

	Health -= DamageAmount;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

	// 1. 데미지를 입을 때마다 '파괴 중(Damaged)' 메시지 브로드캐스트
	FFTMessagePayloadStruct DamagedPayload;
	DamagedPayload.InstigatorActor = DamageCauser;
	DamagedPayload.TargetActor = this;
	DamagedPayload.Value = Health;

	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ShelfDamaged, DamagedPayload);

	// 2. 체력이 0 이하가 되어 파괴되었을 때 '파괴 완료(Destroyed)' 메시지 브로드캐스트
	if (Health <= 0.0f)
	{
		bHasBeenLooted = true;
		UE_LOG(LogFTItem, Log, TEXT("매대 '%s'가 파괴되었습니다! 아이템이 드랍됩니다..."), *GetName());
		
		FFTMessagePayloadStruct DestroyedPayload;
		DestroyedPayload.InstigatorActor = DamageCauser;
		DestroyedPayload.TargetActor = this;
		DestroyedPayload.Value = 0.0f;

		MessageSubsystem.BroadcastMessage(TAG_FT_Event_ShelfDestroyed, DestroyedPayload);

		DropItemsOnFloor();
		Destroy();
	}

	return ActualDamage;
}

void AFTLootShelf::HandleStealCompleted()
{
	if (bHasBeenLooted) return;
	bHasBeenLooted = true;

	UE_LOG(LogFTPlayer, Log, TEXT("LootShelf '%s' 훔치기 완료. 인벤토리에 아이템을 추가합니다."), *GetName());

	GiveStealReward();

	if (bDestroyOnComplete)
	{
		Destroy();
	}
}

void AFTLootShelf::GiveStealReward()
{
	if (!LootItemData) return;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	Payload.ItemId = LootItemData->ItemData.ItemId;
	// 상호작용 주체 정보가 필요하다면 나중에 ChanneledInteraction이나 InteractionComponent에서 받아올 수 있습니다.

	// 설정된 수량만큼 획득 메시지 발송
	for (int32 i = 0; i < LootQuantity; ++i)
	{
		UE_LOG(LogFTItem, Log, TEXT("%d 번째 Itme 획득!"), i + 1);
		MessageSubsystem.BroadcastMessage(TAG_FT_Event_ItemPickedUp, Payload);
	}
}

void AFTLootShelf::DropItemsOnFloor()
{
	if (!LootItemData) return;

	for (int32 i = 0; i < LootQuantity; ++i)
	{
		float RandomX = FMath::FRandRange(-50.0f, 50.0f);
		float RandomY = FMath::FRandRange(-50.0f, 50.0f);
		FVector SpawnLocation = GetActorLocation() + FVector(RandomX, RandomY, 50.0f);
		FRotator SpawnRotation = FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AFTItemActor* NewItem = GetWorld()->SpawnActor<AFTItemActor>(AFTItemActor::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
		if (NewItem)
		{
			NewItem->ItemData = LootItemData;
			NewItem->UpdateAppearance();
		}
	}
}