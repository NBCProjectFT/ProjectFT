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

	// 메타 데이터에 존재하는 아이템인지 확인
	UFTItemDataAsset* ItemDataAsset = FindItemData(ItemId);
	if (!ItemDataAsset)
	{
		UE_LOG(LogFTItem, Warning, TEXT("아이템 획득 실패: 데이터베이스에 Item ID '%s'가 존재하지 않습니다."), *ItemId.ToString());
		return false;
	}

	// 아이템 구조체 정보 저장
	const FTItemDataStruct& Data = ItemDataAsset->ItemData;

	// 무게 한도 검증
	float AddWeight = Data.Weight * Quantity;
	if (CurrentWeight + AddWeight > MaxWeight)
	{
		UE_LOG(LogFTItem, Warning, TEXT("아이템 획득 실패: 인벤토리에 추가 가능한 무게보다 아이템의 무게가 무겁습니다. 현재 무게: %f, 최대 무게: %f, 추가돼야 할 무게: %f"), CurrentWeight, MaxWeight, AddWeight);
		return false;
	}

	// 기존 슬롯이 있으면 누적
	bool bFound = false;
	for (FFTInventoryItem& Slot : Items)
	{
		if (Slot.ItemId == ItemId)
		{
			Slot.Quantity += Quantity;
			bFound = true;
			break;
		}
	}

	// 기존 슬롯이 없으면 새 슬롯 추가
	if (!bFound)
	{
		FFTInventoryItem NewSlot;
		NewSlot.ItemId = ItemId;
		NewSlot.Quantity = Quantity;
		NewSlot.ItemDataAsset = ItemDataAsset;
		Items.Add(NewSlot);
	}

	// 상태 갱신 및 델리게이트 알림
	UpdateWeight();
	OnInventoryChanged.Broadcast();

	UE_LOG(LogFTItem, Log, TEXT("아이템 획득 성공: '%s' %d개를 인벤토리에 추가했습니다."), *ItemId.ToString(), Quantity);
	return true;
}

bool UFTInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
	if (ItemId.IsNone() || Quantity <= 0) return false;

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].ItemId == ItemId)
		{
			if (Items[i].Quantity < Quantity)
			{
				// 가진 개수보다 더 지우려 한 경우
				UE_LOG(LogFTItem, Warning, TEXT("아이템 제거 실패: '%s'를 %d개 소유 중이므로 %d개 제거가 불가능합니다."), *ItemId.ToString(), Items[i].Quantity, Quantity);
				return false;
			}
			
			if (Items[i].Quantity > Quantity)
			{
				Items[i].Quantity -= Quantity;
			}
			else if (Items[i].Quantity == Quantity)
			{
				Items.RemoveAt(i);
			}

			UpdateWeight();
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	UE_LOG(LogFTItem, Warning, TEXT("아이템 제거 실패: Item ID '%s'가 인벤토리에 존재하지 않습니다."), *ItemId.ToString());
	return false;
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

const int32 UFTInventoryComponent::GetItemQuantity(FName ItemId) const 
{
	for (const FFTInventoryItem& slot : Items)
	{
		if (slot.ItemId == ItemId)
		{
			return slot.Quantity;
		}
	}
	
	return 0;
}

const UFTItemDataAsset* UFTInventoryComponent::GetItemPtr(FName ItemId) const
{
	for (const FFTInventoryItem& slot : Items)
	{
		if (slot.ItemId == ItemId)
		{
			return slot.ItemDataAsset.Get();
		}
	}
	
	return nullptr;
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
