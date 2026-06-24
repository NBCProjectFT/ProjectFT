// Fill out your copyright notice in the Description page of Project Settings.

#include "FTInventoryComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "Engine/AssetManager.h"

UFTInventoryComponent::UFTInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// GameplayMessageSubsystem 구독 등록
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageListenerHandle = MessageSubsystem.RegisterListener<FFTMessagePayloadStruct>(
		TAG_FT_Event_ItemPickedUp,
		this,
		&UFTInventoryComponent::HandleItemPickedUpMessage
	);

	UpdateWeight();
}

void UFTInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 구독 해제 안전 처리
	if (MessageListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		MessageSubsystem.UnregisterListener(MessageListenerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

bool UFTInventoryComponent::AddItem(FName ItemId, int32 Quantity)
{
	if (ItemId.IsNone() || Quantity <= 0) return false;

	UFTItemDataAsset* ItemDataAsset = FindItemData(ItemId);
	if (!ItemDataAsset)
	{
		UE_LOG(LogFTItem, Warning, TEXT("AddItem Fail: Item ID '%s' is not registered in the database!"), *ItemId.ToString());
		return false;
	}

	const FTItemDataStruct& Data = ItemDataAsset->ItemData;

	// 1. 무게 한도 검증
	float AddWeight = Data.Weight * Quantity;
	if (CurrentWeight + AddWeight > MaxWeight)
	{
		UE_LOG(LogFTItem, Warning, TEXT("AddItem Fail: Exceeded weight capacity. Current: %f, Max: %f, Adding: %f"), CurrentWeight, MaxWeight, AddWeight);
		return false;
	}

	int32 RemainingQuantity = Quantity;

	// 2. 기존 스택에 중첩 저장 시도 (MaxStack > 1인 경우)
	if (Data.MaxStack > 1)
	{
		for (FFTInventoryItem& Slot : Items)
		{
			if (Slot.ItemId == ItemId && Slot.Quantity < Data.MaxStack)
			{
				int32 AvailableSpace = Data.MaxStack - Slot.Quantity;
				int32 AmountToAdd = FMath::Min(RemainingQuantity, AvailableSpace);

				Slot.Quantity += AmountToAdd;
				RemainingQuantity -= AmountToAdd;

				if (RemainingQuantity <= 0) break;
			}
		}
	}

	// 3. 남은 아이템은 새로운 인벤토리 슬롯에 보관
	while (RemainingQuantity > 0)
	{
		int32 AmountToSlot = FMath::Min(RemainingQuantity, Data.MaxStack);

		FFTInventoryItem NewSlot;
		NewSlot.ItemId = ItemId;
		NewSlot.Quantity = AmountToSlot;
		NewSlot.ItemDataAsset = ItemDataAsset;

		Items.Add(NewSlot);
		RemainingQuantity -= AmountToSlot;
	}

	// 4. 상태 갱신 및 델리게이트 알림
	UpdateWeight();
	OnInventoryChanged.Broadcast();

	UE_LOG(LogFTItem, Log, TEXT("AddItem Success: %d of '%s' added to Inventory."), Quantity, *ItemId.ToString());
	return true;
}

bool UFTInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
	if (ItemId.IsNone() || Quantity <= 0) return false;

	// 역순으로 탐색하여 수량 제거 (끝쪽 슬롯부터 소모하는 방식)
	int32 RemainingToRemove = Quantity;
	for (int32 i = Items.Num() - 1; i >= 0; --i)
	{
		if (Items[i].ItemId == ItemId)
		{
			if (Items[i].Quantity > RemainingToRemove)
			{
				Items[i].Quantity -= RemainingToRemove;
				RemainingToRemove = 0;
				break;
			}
			else
			{
				RemainingToRemove -= Items[i].Quantity;
				Items.RemoveAt(i); // 슬롯 비우기
			}
		}
	}

	if (RemainingToRemove > 0)
	{
		// 완벽하게 다 지우지 못한 경우 원래 가지고 있던 양보다 많이 지우려 시도함
		UE_LOG(LogFTItem, Warning, TEXT("RemoveItem: Attempted to remove %d of '%s' but only removed remaining %d."), Quantity, *ItemId.ToString(), Quantity - RemainingToRemove);
		UpdateWeight();
		OnInventoryChanged.Broadcast();
		return false;
	}

	UpdateWeight();
	OnInventoryChanged.Broadcast();
	return true;
}

void UFTInventoryComponent::ClearInventory()
{
	Items.Empty();
	UpdateWeight();
	OnInventoryChanged.Broadcast();
}

void UFTInventoryComponent::SetMaxWeight(float NewMaxWeight)
{
	MaxWeight = FMath::Max(0.0f, NewMaxWeight);
	OnInventoryChanged.Broadcast();
}

void UFTInventoryComponent::UpdateWeight()
{
	float NewWeight = 0.0f;
	for (const FFTInventoryItem& Slot : Items)
	{
		if (Slot.ItemDataAsset)
		{
			NewWeight += Slot.ItemDataAsset->ItemData.Weight * Slot.Quantity;
		}
	}
	CurrentWeight = NewWeight;
}

UFTItemDataAsset* UFTInventoryComponent::FindItemData(FName ItemId) const
{
	UAssetManager& AssetManager = UAssetManager::Get();

	// [디버깅 로그] 에셋 매니저에 스캔된 모든 FTItemItem 목록 출력
	TArray<FPrimaryAssetId> IdList;
	AssetManager.GetPrimaryAssetIdList(FName("FTItemItem"), IdList);
	UE_LOG(LogFTItem, Warning, TEXT("=== Asset Manager 'FTItemItem' List (Total: %d) ==="), IdList.Num());
	for (const FPrimaryAssetId& Id : IdList)
	{
		UE_LOG(LogFTItem, Warning, TEXT("  - Found AssetId: %s (Name: %s)"), *Id.ToString(), *Id.PrimaryAssetName.ToString());
	}

	FPrimaryAssetId AssetId = FPrimaryAssetId(FName("FTItemItem"), ItemId);
	
	// 1. 이미 메모리에 로드되어 있는지 확인
	UObject* AssetObj = AssetManager.GetPrimaryAssetObject(AssetId);
	if (!AssetObj)
	{
		// 2. 로드되어 있지 않다면 에셋 매니저가 스캔한 경로를 통해 동기식으로 로드(Fallback)
		FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
		UE_LOG(LogFTItem, Warning, TEXT("  - Searching ItemId: %s -> Path: %s"), *ItemId.ToString(), *AssetPath.ToString());
		if (AssetPath.IsValid())
		{
			AssetObj = AssetPath.TryLoad();
		}
	}
	
	return Cast<UFTItemDataAsset>(AssetObj);
}

void UFTInventoryComponent::HandleItemPickedUpMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	AActor* Owner = GetOwner();

	// 메시지 발신 주체 혹은 타겟이 이 컴포넌트의 소유주(플레이어)인지 판별하여 본인 습득물만 추가
	if (Payload.TargetActor == Owner || Payload.InstigatorActor == Owner ||
		(Payload.TargetActor == nullptr && Payload.InstigatorActor == nullptr))
	{
		UE_LOG(LogFTItem, Log, TEXT("Inventory Component received Event.Item.PickedUp message for Item: %s"), *Payload.ItemId.ToString());
		AddItem(Payload.ItemId, 1);
	}
}
