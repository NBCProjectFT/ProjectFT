// Fill out your copyright notice in the Description page of Project Settings.

#include "FTLootShelf.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

#include "ProjectFT/Components/FTChanneledInteractionComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"

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

void AFTLootShelf::HandleStealCompleted()
{
	UE_LOG(LogFTPlayer, Log, TEXT("LootShelf '%s' 훔치기 완료."), *GetName());

	// TODO: 실제로는 여기서 아이템 지급/인벤토리 추가 등을 처리한다(예: GameplayMessageRouter로 이벤트 발행).
	if (bDestroyOnComplete)
	{
		Destroy();
	}
}
