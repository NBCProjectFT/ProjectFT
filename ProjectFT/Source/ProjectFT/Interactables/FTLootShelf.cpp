#include "FTLootShelf.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Components/FTChanneledInteractionComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"

#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTLootShelfDataAsset.h"
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

	// 이 채널은 "훔치기"다 — 진행 중 시전자(플레이어) ASC에 State.Stealing을 부여하게 지정한다(AI 도둑질 인식용).
	// EditAnywhere라 인스턴스/BP에서 덮어쓸 수 있다.
	ChanneledInteraction->ChannelingStateTag = TAG_FT_State_Stealing;
}

void AFTLootShelf::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	InitializeFromDataAsset();
}

void AFTLootShelf::InitializeFromDataAsset()
{
	if (!ShelfDataAsset) return;

	// Static Mesh 설정
	if (ShelfMesh)
	{
		UStaticMesh* TargetMesh = (bIsOnCooldown && ShelfDataAsset->CooldownMesh) ? ShelfDataAsset->CooldownMesh : ShelfDataAsset->ShelfMesh;
		if (TargetMesh)
		{
			ShelfMesh->SetStaticMesh(TargetMesh);
		}
	}

	// 내구도 설정
	Health = ShelfDataAsset->MaxHealth;

	if (ChanneledInteraction)
	{
		ChanneledInteraction->SetRequiredSeconds(ShelfDataAsset->RequiredSeconds);
	}
}

void AFTLootShelf::BeginPlay()
{
	Super::BeginPlay();

	InitializeFromDataAsset();

	// 게이지 완료 시 훔치기 성공 처리.
	if (ChanneledInteraction)
	{
		ChanneledInteraction->SetActive(true);
		ChanneledInteraction->OnCompleted.AddDynamic(this, &AFTLootShelf::HandleStealCompleted);
	}
}

FText AFTLootShelf::GetInteractionPrompt_Implementation() const
{
	if (bIsOnCooldown)
	{
		if (ShelfDataAsset && !ShelfDataAsset->CooldownPrompt.IsEmpty())
		{
			return ShelfDataAsset->CooldownPrompt;
		}
		return FText::FromString(TEXT("재충전 중..."));
	}

	if (ShelfDataAsset && !ShelfDataAsset->InteractionPrompt.IsEmpty())
	{
		return ShelfDataAsset->InteractionPrompt;
	}
	return FText::FromString(TEXT("훔치기"));
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

		// 아이템 스폰 시 충돌 튕김(하늘로 날아가는 현상) 방지를 위해 매대 콜리전 비활성화
		if (ShelfMesh)
		{
			ShelfMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ShelfMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}

		UE_LOG(LogFTItem, Log, TEXT("매대 '%s'가 파괴되었습니다! 아이템이 드랍됩니다..."), *GetName());
		
		FFTMessagePayloadStruct DestroyedPayload;
		DestroyedPayload.InstigatorActor = DamageCauser;
		DestroyedPayload.TargetActor = this;
		DestroyedPayload.Value = 0.0f;

		MessageSubsystem.BroadcastMessage(TAG_FT_Event_ShelfDestroyed, DestroyedPayload);

		// 쿨다운 중이 아닐 때만 아이템 드롭 (쿨다운 중인 매대는 텅 비어 있음)
		if (!bIsOnCooldown)
		{
			DropItemsOnFloor();
		}
		
		Destroy();
	}

	return ActualDamage;
}

void AFTLootShelf::HandleStealCompleted()
{
	if (bHasBeenLooted || bIsOnCooldown) return;

	UE_LOG(LogFTPlayer, Log, TEXT("LootShelf '%s' 훔치기 완료. 인벤토리에 아이템을 추가합니다."), *GetName());

	GiveStealReward();
	
	FFTMessagePayloadStruct StealPayload;
	
	StealPayload.TargetActor = this;
	StealPayload.Value = 1.0f;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		TAG_FT_Event_StealCompleted,
		StealPayload
	);
	
	// 데이터 에셋이 유효하고 쿨다운 대기 시간이 설정된 경우 쿨다운 상태로 이행
	if (ShelfDataAsset && ShelfDataAsset->CooldownSeconds > 0.0f)
	{
		StartInteractionCooldown(ShelfDataAsset->CooldownSeconds);
	}
	else
	{
		bHasBeenLooted = true;
		
		const bool bShouldDestroy = ShelfDataAsset ? ShelfDataAsset->bDestroyOnComplete : true;
		if (bShouldDestroy)
		{
			Destroy();
		}
	}
}

void AFTLootShelf::StartInteractionCooldown(float CooldownDuration)
{
	bIsOnCooldown = true;

	// 상호작용 컴포넌트 비활성화
	if (ChanneledInteraction)
	{
		ChanneledInteraction->SetActive(false);
	}

	// 쿨다운 시 전용 메시로 교체
	if (ShelfDataAsset && ShelfDataAsset->CooldownMesh && ShelfMesh)
	{
		ShelfMesh->SetStaticMesh(ShelfDataAsset->CooldownMesh);
	}

	// 쿨다운 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &AFTLootShelf::EndInteractionCooldown, CooldownDuration, false);
}

void AFTLootShelf::EndInteractionCooldown()
{
	bIsOnCooldown = false;

	// 상호작용 컴포넌트 재활성화
	if (ChanneledInteraction)
	{
		ChanneledInteraction->SetActive(true);
	}

	// 원래 메시로 복원
	if (ShelfDataAsset && ShelfDataAsset->ShelfMesh && ShelfMesh)
	{
		ShelfMesh->SetStaticMesh(ShelfDataAsset->ShelfMesh);
	}
}

class UFTItemDataAsset* AFTLootShelf::GetRandomLootItem(int32& OutQuantity) const
{
	UFTItemDataAsset* SelectedItem = SelectRandomItemFromPool();
	if (SelectedItem)
	{
		OutQuantity = FMath::RandRange(ShelfDataAsset->LootQuantityMin, ShelfDataAsset->LootQuantityMax);
	}
	else
	{
		OutQuantity = 0;
	}
	return SelectedItem;
}

class UFTItemDataAsset* AFTLootShelf::SelectRandomItemFromPool() const
{
	if (!ShelfDataAsset || ShelfDataAsset->PossibleLootItems.Num() == 0)
	{
		return nullptr;
	}

	// 1. 총 가중치 계산
	int32 TotalWeight = 0;
	for (const FFTLootShelfItemRow& Row : ShelfDataAsset->PossibleLootItems)
	{
		if (Row.ItemDataAsset)
		{
			TotalWeight += FMath::Max(0, Row.Weight);
		}
	}

	if (TotalWeight <= 0)
	{
		return nullptr;
	}

	// 2. 임의의 가중치 값 결정
	int32 RandomValue = FMath::RandRange(0, TotalWeight - 1);
	int32 CurrentWeightSum = 0;

	// 3. 누적 가중치 영역에 매칭되는 아이템 에셋 반환
	for (const FFTLootShelfItemRow& Row : ShelfDataAsset->PossibleLootItems)
	{
		if (Row.ItemDataAsset)
		{
			CurrentWeightSum += Row.Weight;
			if (RandomValue < CurrentWeightSum)
			{
				return Row.ItemDataAsset;
			}
		}
	}

	return nullptr;
}

void AFTLootShelf::GiveStealReward()
{
	int32 Quantity = 0;
	UFTItemDataAsset* SelectedItem = GetRandomLootItem(Quantity);
	if (!SelectedItem) return;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	Payload.ItemId = SelectedItem->ItemData.ItemId;

	// 설정된 수량만큼 획득 메시지 발송
	for (int32 i = 0; i < Quantity; ++i)
	{
		UE_LOG(LogFTItem, Log, TEXT("%d 번째 Item 획득!"), i + 1);
		MessageSubsystem.BroadcastMessage(TAG_FT_Event_ItemPickedUp, Payload);
	}
}

void AFTLootShelf::DropItemsOnFloor()
{
	if (ShelfDataAsset && ShelfDataAsset->PossibleLootItems.Num() > 0)
	{
		int32 Quantity = FMath::RandRange(ShelfDataAsset->LootQuantityMin, ShelfDataAsset->LootQuantityMax);
		for (int32 i = 0; i < Quantity; ++i)
		{
			UFTItemDataAsset* SelectedItem = SelectRandomItemFromPool();
			if (SelectedItem)
			{
				SpawnItemActor(SelectedItem);
			}
		}
	}
}

void AFTLootShelf::SpawnItemActor(UFTItemDataAsset* ItemDataAsset)
{
	if (!ItemDataAsset) return;

	float RandomX = FMath::FRandRange(-50.0f, 50.0f);
	float RandomY = FMath::FRandRange(-50.0f, 50.0f);
	FVector SpawnLocation = GetActorLocation() + FVector(RandomX, RandomY, 50.0f);
	FRotator SpawnRotation = FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFTItemActor* NewItem = GetWorld()->SpawnActor<AFTItemActor>(AFTItemActor::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
	if (NewItem)
	{
		NewItem->ItemData = ItemDataAsset;
		NewItem->UpdateAppearance();
	}
}