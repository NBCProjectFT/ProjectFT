#include "FTItemPoolSubsystem.h"
#include "FTItemActor.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "FTItemFunctionLibrary.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Components/WidgetComponent.h"
#include "InputCoreTypes.h"
#include "ProjectFT/Player/FTPlayerCharacter.h"
#include "ProjectFT/Interactables/FTLootShelf.h"

void UFTItemPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bWasScanKeyDown = false;
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

		// 모든 아이템 에셋 비동기 프리로딩으로 런타임 렉(TryLoad) 방지
		UAssetManager& AssetManager = UAssetManager::Get();
		TArray<FPrimaryAssetId> IdList;
		AssetManager.GetPrimaryAssetIdList(FName("FTItemItem"), IdList);
		if (IdList.Num() > 0)
		{
			AssetManager.LoadPrimaryAssets(IdList, TArray<FName>(), FStreamableDelegate());
			UE_LOG(LogFTItem, Log, TEXT("UFTItemPoolSubsystem: %d개의 아이템 에셋 비동기 프리로딩 시작"), IdList.Num());
		}
	}
}

void UFTItemPoolSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewUpdateTimerHandle);
	}
	ActiveItemActors.Empty();

	// UGameplayMessageSubsystem(GameInstanceSubsystem)은 월드 소멸(Deinitialize) 시점에 이미 내부 Router가 파괴되어 정리 중일 수 있습니다.
	// 이 단계에서 명시적으로 UnregisterListener를 호출하면 어설션 크래시가 발생하므로 생략합니다.
	// 어차피 서브시스템과 월드가 통째로 해제되는 시점이므로 누수가 발생하지 않습니다.

	// 대기 중인 풀 내부의 모든 액터 안전 소멸
	for (auto& Pair : InactivePoolsMap)
	{
		for (TObjectPtr<AFTItemActor> Actor : Pair.Value.Actors)
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
	}
	InactivePoolsMap.Empty();

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

	AFTItemActor* TargetActor = nullptr;

	// 1. 해당 ItemId의 풀이 존재하고 재사용 가능한 액터가 있는지 확인
	if (InactivePoolsMap.Contains(ItemId))
	{
		TArray<TObjectPtr<AFTItemActor>>& ActorsList = InactivePoolsMap[ItemId].Actors;
		while (ActorsList.Num() > 0)
		{
			TObjectPtr<AFTItemActor> PooledActor = ActorsList.Pop();
			if (IsValid(PooledActor))
			{
				TargetActor = PooledActor;
				break;
			}
		}
	}

	if (TargetActor)
	{
		// 2. 풀에서 꺼낸 액터 활성화 (메시가 이미 해당 아이템에 맞게 셋팅되어 있으므로 UpdateAppearance() 생략!)
		TargetActor->SetActorLocationAndRotation(Location, Rotation);

		TargetActor->SetActorHiddenInGame(false);
		TargetActor->SetActorTickEnabled(true);

		// 물리 및 콜리전 재설정
		TargetActor->SetupPhysicsAndCollision();

		UE_LOG(LogFTItem, Log, TEXT("아이템 풀 재사용 성공 (동일 메시 재사용): '%s' 획득 완료"), *ItemId.ToString());
	}
	else
	{
		// 3. 풀이 비어있으면 새로 생성
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		TargetActor = World->SpawnActor<AFTItemActor>(AFTItemActor::StaticClass(), Location, Rotation, SpawnParams);
		if (TargetActor)
		{
			TargetActor->ItemData = ItemDataAsset;
			TargetActor->UpdateAppearance();

			UE_LOG(LogFTItem, Log, TEXT("아이템 풀 신규 스폰 (새로운 메시 설정): '%s' 생성 완료"), *ItemId.ToString());
		}
	}

	if (TargetActor)
	{
		RegisterActiveItem(TargetActor);
	}

	return TargetActor;
}

void UFTItemPoolSubsystem::ReleaseItemActor(AFTItemActor* ItemActor)
{
	if (!ItemActor) return;

	ActiveItemActors.Remove(ItemActor);
	ItemActor->SetTooltipVisibility(false);

	if (ActiveItemActors.Num() == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(PreviewUpdateTimerHandle);
	}

	FName ItemId = (ItemActor->ItemData) ? ItemActor->ItemData->ItemData.ItemId : NAME_None;
	if (ItemId.IsNone())
	{
		ItemActor->Destroy();
		return;
	}

	// 개별 아이템 풀 한도 체크 (예: 동일 아이템 종류는 최대 15개까지만 풀링 보관)
	FFTItemActorArray& Pool = InactivePoolsMap.FindOrAdd(ItemId);
	if (Pool.Actors.Num() >= 15)
	{
		UE_LOG(LogFTItem, Log, TEXT("아이템 풀 포화 상태 (ItemId: '%s', 현재 크기: %d): 액터 '%s'를 실제 Destroy 처리합니다."), *ItemId.ToString(), Pool.Actors.Num(), *ItemActor->GetName());
		ItemActor->Destroy();
		return;
	}

	// 액터 비활성화 처리
	ItemActor->SetActorHiddenInGame(true);
	ItemActor->SetActorTickEnabled(false);

	// 물리 시뮬레이션 및 콜리전 끄기 (메시는 유지!)
	if (UStaticMeshComponent* MeshComp = ItemActor->FindComponentByClass<UStaticMeshComponent>())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		MeshComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	// 안전을 위해 멀리 떨어진 공간으로 위치 이동
	ItemActor->SetActorLocation(FVector(0.0f, 0.0f, -99999.0f));

	Pool.Actors.Add(ItemActor);
	UE_LOG(LogFTItem, Log, TEXT("아이템 풀 반환 완료: '%s' (ItemId: '%s', 현재 풀 크기: %d)"), *ItemActor->GetName(), *ItemId.ToString(), Pool.Actors.Num());
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
	return UFTItemFunctionLibrary::CalculateDropLocation(InstigatorActor);
}

UFTItemDataAsset* UFTItemPoolSubsystem::FindItemData(FName ItemId) const
{
	return UFTItemFunctionLibrary::FindItemData(this, ItemId);
}

void UFTItemPoolSubsystem::UpdateAllItemPreviews()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn) return;

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector CameraForward = CameraRotation.Vector();

	// 1. 화면 정중앙(에임)에 조준 중인 액터 조회 (Line Trace)
	AActor* ActorUnderAim = nullptr;
	FHitResult AimHit;
	FVector TraceEnd = CameraLocation + (CameraForward * PreviewMaxDistance);
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(PlayerPawn);
	if (GetWorld()->LineTraceSingleByChannel(AimHit, CameraLocation, TraceEnd, ECC_Visibility, TraceParams))
	{
		ActorUnderAim = AimHit.GetActor();
	}

	// 2. 스캔 키(마우스 좌클릭)가 꾹 눌려 있는지 감지
	const bool bIsScanKeyDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);


	// 💡 포스건 장착 상태 및 매대 스캔 조건 검사
	AFTLootShelf* AimedShelf = Cast<AFTLootShelf>(ActorUnderAim);
	AFTPlayerCharacter* FTPlayer = Cast<AFTPlayerCharacter>(PlayerPawn);
	const bool bIsHoldingForceGun = FTPlayer && (FTPlayer->GetCurrentHeldInventoryItem().ItemId == FName("ID_Weapon_ForceGun"));

	// 💡 스캔 키(마우스 좌클릭)의 상태 전이(Edge Trigger)를 감지하여 포스건 장착 상태일 때 정식 아이템 사용 어빌리티 격발
	if (bIsHoldingForceGun)
	{
		if (bIsScanKeyDown && !bWasScanKeyDown)
		{
			FTPlayer->HandleUseItemPressed();
		}
		else if (!bIsScanKeyDown && bWasScanKeyDown)
		{
			FTPlayer->HandleUseItemReleased();
		}
	}
	bWasScanKeyDown = bIsScanKeyDown;

	const bool bIsScanning = AimedShelf && bIsHoldingForceGun && bIsScanKeyDown;

	if (bIsScanning)
	{
		if (LastScannedShelfActor != AimedShelf)
		{
			LastScannedShelfActor = AimedShelf;

			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
			FFTMessagePayloadStruct Payload;
			Payload.InstigatorActor = PlayerPawn;
			Payload.TargetActor = AimedShelf;
			Payload.Value = 1.0f; // 1.0f: 스캔 시작 및 UI 활성화 통지

			MessageSubsystem.BroadcastMessage(TAG_FT_Event_ForceGunScan, Payload);
		}
	}
	else
	{
		if (LastScannedShelfActor != nullptr)
		{
			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
			FFTMessagePayloadStruct Payload;
			Payload.InstigatorActor = PlayerPawn;
			Payload.TargetActor = LastScannedShelfActor;
			Payload.Value = 0.0f; // 0.0f: 스캔 중지 및 UI 비활성화 통지

			MessageSubsystem.BroadcastMessage(TAG_FT_Event_ForceGunScan, Payload);

			LastScannedShelfActor = nullptr;
		}
	}

	TArray<TObjectPtr<AFTItemActor>> ToRemove;

	for (TObjectPtr<AFTItemActor> Item : ActiveItemActors)
	{
		if (!IsValid(Item))
		{
			ToRemove.Add(Item);
			continue;
		}

		// 캐릭터의 손에 쥐여져 어태치(Attach)되어 있는 아이템 비주얼은 시야 감지에서 스킵합니다.
		if (Item->GetAttachParentActor() != nullptr)
		{
			Item->SetTooltipVisibility(false);
			continue;
		}

		FVector ItemLocation = Item->GetActorLocation();

		// 기본 거리 검사 (반드시 특정 거리 이내에 있어야 함)
		float Distance = FVector::Dist(CameraLocation, ItemLocation);
		if (Distance > PreviewMaxDistance)
		{
			Item->SetTooltipVisibility(false);
			continue;
		}

		// 3. 크로스헤어 호버 조건 만족 시 미리보기 노출
		if (Item == ActorUnderAim)
		{
			Item->SetTooltipVisibility(true);
		}
		else
		{
			Item->SetTooltipVisibility(false);
		}
	}

	// 유효하지 않은 액터 제거
	for (TObjectPtr<AFTItemActor> DeadItem : ToRemove)
	{
		ActiveItemActors.Remove(DeadItem);
	}

	if (ActiveItemActors.Num() == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(PreviewUpdateTimerHandle);
	}
}

void UFTItemPoolSubsystem::RegisterActiveItem(AFTItemActor* ItemActor)
{
	if (!ItemActor) return;

	ActiveItemActors.AddUnique(ItemActor);

	if (ActiveItemActors.Num() == 1)
	{
		GetWorld()->GetTimerManager().SetTimer(PreviewUpdateTimerHandle, this, &UFTItemPoolSubsystem::UpdateAllItemPreviews, 0.15f, true);
	}
}
