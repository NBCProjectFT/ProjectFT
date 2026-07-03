#include "FTItemPoolSubsystem.h"
#include "FTItemActor.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"

void UFTItemPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogFTItem, Log, TEXT("UFTItemPoolSubsystem 초기화 완료"));
}

void UFTItemPoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.IsGameWorld())
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		
		// 드롭 요청 메시지 구독 등록
		DropItemListenerHandle = MessageSubsystem.RegisterListener<FFTMessagePayloadStruct>(
			TAG_FT_Request_DropItem,
			this,
			&UFTItemPoolSubsystem::HandleDropItemMessage
		);
		
		UE_LOG(LogFTItem, Log, TEXT("UFTItemPoolSubsystem: DropItem 요청 구독 등록 완료 (Game World)"));
	}
}

void UFTItemPoolSubsystem::Deinitialize()
{
	// UGameplayMessageSubsystem(GameInstanceSubsystem)은 월드 소멸(Deinitialize) 시점에 이미 내부 Router가 파괴되어 정리 중일 수 있습니다.
	// 이 단계에서 명시적으로 UnregisterListener를 호출하면 어설션 크래시가 발생하므로 생략합니다.
	// 어차피 서브시스템과 월드가 통째로 해제되는 시점이므로 누수가 발생하지 않습니다.

	Super::Deinitialize();
}

AFTItemActor* UFTItemPoolSubsystem::AcquireItemActor(FName ItemId, const FVector& Location, const FRotator& Rotation)
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	UFTItemDataAsset* ItemDataAsset = FindItemData(ItemId);
	if (!ItemDataAsset)
	{
		UE_LOG(LogFTItem, Warning, TEXT("AcquireItemActor 실패: Item ID '%s'의 에셋을 찾을 수 없습니다."), *ItemId.ToString());
		return nullptr;
	}

	// [1단계] 풀링이 구현되지 않은 기본 상태이므로 매번 SpawnActor 수행
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	AFTItemActor* NewActor = World->SpawnActor<AFTItemActor>(AFTItemActor::StaticClass(), Location, Rotation, SpawnParams);
	if (NewActor)
	{
		NewActor->ItemData = ItemDataAsset;
		NewActor->UpdateAppearance();
		
		UE_LOG(LogFTItem, Log, TEXT("아이템 스폰 성공: '%s' 생성 완료"), *ItemId.ToString());
	}

	return NewActor;
}

void UFTItemPoolSubsystem::ReleaseItemActor(AFTItemActor* ItemActor)
{
	if (!ItemActor) return;

	// [1단계] 풀링이 구현되지 않은 기본 상태이므로 즉시 Destroy
	ItemActor->Destroy();
}

void UFTItemPoolSubsystem::HandleDropItemMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	AActor* Instigator = Payload.InstigatorActor;
	if (!Instigator)
	{
		UE_LOG(LogFTItem, Warning, TEXT("HandleDropItemMessage 실패: InstigatorActor가 유효하지 않습니다."));
		return;
	}

	int32 DropQuantity = FMath::Max(1, static_cast<int32>(Payload.Value));
	UE_LOG(LogFTItem, Log, TEXT("HandleDropItemMessage 수신: ItemId '%s', 드롭 수량: %d"), *Payload.ItemId.ToString(), DropQuantity);

	for (int32 i = 0; i < DropQuantity; ++i)
	{
		FVector DropLoc = CalculateDropLocation(Instigator);
		FRotator DropRot = FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f); // 랜덤 방향 회전값
		AcquireItemActor(Payload.ItemId, DropLoc, DropRot);
	}
}

FVector UFTItemPoolSubsystem::CalculateDropLocation(AActor* InstigatorActor) const
{
	if (!InstigatorActor) return FVector::ZeroVector;

	FVector StartLoc = InstigatorActor->GetActorLocation();
	FVector ForwardDir = InstigatorActor->GetActorForwardVector();

	// 전방 120cm, 좌우 -30cm ~ 30cm 오프셋
	FVector DropOffset = (ForwardDir * 120.0f) + (InstigatorActor->GetActorRightVector() * FMath::FRandRange(-30.0f, 30.0f));
	FVector TargetLoc = StartLoc + DropOffset;

	FHitResult HitResult;
	FVector TraceStart = TargetLoc + FVector(0.0f, 0.0f, 50.0f);
	FVector TraceEnd = TargetLoc - FVector(0.0f, 0.0f, 500.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(InstigatorActor);

	UWorld* World = GetWorld();
	if (World && World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return HitResult.Location + FVector(0.0f, 0.0f, 15.0f);
	}

	return TargetLoc;
}

UFTItemDataAsset* UFTItemPoolSubsystem::FindItemData(FName ItemId) const
{
	UAssetManager& AssetManager = UAssetManager::Get();
	FPrimaryAssetId AssetId = FPrimaryAssetId(FName("FTItemItem"), ItemId);
	
	UObject* AssetObj = AssetManager.GetPrimaryAssetObject(AssetId);
	if (!AssetObj)
	{
		FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
		if (AssetPath.IsValid())
		{
			AssetObj = AssetPath.TryLoad();
		}
	}
	
	return Cast<UFTItemDataAsset>(AssetObj);
}
