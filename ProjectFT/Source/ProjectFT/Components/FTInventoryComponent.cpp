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

	// 퀵슬롯 크기를 6개로 초기화 (1~6번 슬롯)
	QuickSlots.Init(NAME_None, 6);

	// GameplayMessageSubsystem 구독 등록
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	
	// 아이템 획득 메시지 리스너
	PickedUpListenerHandle = MessageSubsystem.RegisterListener<FFTMessagePayloadStruct>(
		TAG_FT_Event_ItemPickedUp,
		this,
		&UFTInventoryComponent::HandleItemPickedUpMessage
	);

	// 아이템 사용 메시지 리스너
	ConsumedListenerHandle = MessageSubsystem.RegisterListener<FFTMessagePayloadStruct>(
		TAG_FT_Event_ItemConsumed,
		this,
		&UFTInventoryComponent::HandleItemConsumedMessage
	);

	UpdateWeight();
}

void UFTInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

	// 구독 해제 안전 처리
	if (PickedUpListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(PickedUpListenerHandle);
	}

	if (ConsumedListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(ConsumedListenerHandle);
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

			// 소모 후 해당 아이템의 보유량이 0이 되면 퀵슬롯에서도 등록 해제
			if (GetItemQuantity(ItemId) <= 0)
			{
				for (int32 SlotIdx = 0; SlotIdx < QuickSlots.Num(); ++SlotIdx)
				{
					if (QuickSlots[SlotIdx] == ItemId)
					{
						QuickSlots[SlotIdx] = NAME_None;
						UE_LOG(LogFTItem, Log, TEXT("아이템 보유량 0 도달: 퀵슬롯 %d번에서 '%s' 제거 완료"), SlotIdx, *ItemId.ToString());
					}
				}
			}

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

	// 인벤토리 초기화 시 퀵슬롯도 전체 해제
	for (int32 i = 0; i < QuickSlots.Num(); ++i)
	{
		QuickSlots[i] = NAME_None;
	}

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
	//UE_LOG(LogFTItem, Warning, TEXT("=== 에셋 매니저 'FTItemItem' 목록 (총: %d개) ==="), IdList.Num());
	//for (const FPrimaryAssetId& Id : IdList)
	//{
	//	UE_LOG(LogFTItem, Warning, TEXT("  - 발견된 AssetId: %s (이름: %s)"), *Id.ToString(), *Id.PrimaryAssetName.ToString());
	//}

	FPrimaryAssetId AssetId = FPrimaryAssetId(FName("FTItemItem"), ItemId);
	
	// 1. 이미 메모리에 로드되어 있는지 확인
	UObject* AssetObj = AssetManager.GetPrimaryAssetObject(AssetId);
	if (!AssetObj)
	{
		// 2. 로드되어 있지 않다면 에셋 매니저가 스캔한 경로를 통해 동기식으로 로드(Fallback)
		FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
		UE_LOG(LogFTItem, Warning, TEXT("  - ItemId 검색 중: %s -> 경로: %s"), *ItemId.ToString(), *AssetPath.ToString());
		if (AssetPath.IsValid())
		{
			AssetObj = AssetPath.TryLoad();
		}
	}
	
	return Cast<UFTItemDataAsset>(AssetObj);
}

bool UFTInventoryComponent::SetQuickSlot(int32 SlotIndex, FName ItemId)
{
	if (!QuickSlots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogFTItem, Warning, TEXT("퀵슬롯 등록 실패: 유효하지 않은 슬롯 인덱스 %d"), SlotIndex);
		return false;
	}

	// 퀵슬롯 비우기 요청인 경우
	if (ItemId.IsNone())
	{
		QuickSlots[SlotIndex] = NAME_None;
		OnInventoryChanged.Broadcast();
		return true;
	}

	// 중복 등록 방지: 이미 다른 슬롯에 등록되어 있다면 해제
	for (int32 i = 0; i < QuickSlots.Num(); ++i)
	{
		if (QuickSlots[i] == ItemId)
		{
			QuickSlots[i] = NAME_None;
		}
	}

	// 인벤토리에 보유 중인지 확인
	if (GetItemQuantity(ItemId) <= 0)
	{
		UE_LOG(LogFTItem, Warning, TEXT("퀵슬롯 등록 실패: 인벤토리에 보유하고 있지 않은 아이템입니다. (Item ID: '%s')"), *ItemId.ToString());
		return false;
	}

	// 아이템 데이터 확인
	UFTItemDataAsset* ItemDataAsset = FindItemData(ItemId);
	if (!ItemDataAsset)
	{
		UE_LOG(LogFTItem, Warning, TEXT("퀵슬롯 등록 실패: Item ID '%s'에 해당하는 에셋을 찾을 수 없습니다."), *ItemId.ToString());
		return false;
	}

	// 일반(Common) 또는 None 카테고리 아이템은 등록 차단
	EFTItemCategoryType Category = ItemDataAsset->ItemData.CategoryType;
	if (Category == EFTItemCategoryType::Common || Category == EFTItemCategoryType::None)
	{
		UE_LOG(LogFTItem, Warning, TEXT("퀵슬롯 등록 실패: 일반(Common) 또는 카테고리가 지정되지 않은 아이템은 등록할 수 없습니다. (Item ID: '%s')"), *ItemId.ToString());
		return false;
	}

	QuickSlots[SlotIndex] = ItemId;
	OnInventoryChanged.Broadcast();
	
	UE_LOG(LogFTItem, Log, TEXT("퀵슬롯 %d번에 아이템 '%s' 등록 완료"), SlotIndex, *ItemId.ToString());
	return true;
}

bool UFTInventoryComponent::GetQuickSlotItem(int32 SlotIndex, FFTInventoryItem& OutItem) const
{
	if (!QuickSlots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogFTItem, Warning, TEXT("GetQuickSlotItem 호출 실패: 유효하지 않은 슬롯 인덱스 %d"), SlotIndex);
		return false;
	}

	FName ItemId = QuickSlots[SlotIndex];
	if (ItemId.IsNone())
	{
		UE_LOG(LogFTItem, Warning, TEXT("GetQuickSlotItem 호출 실패: 퀵슬롯 %d번이 비어 있습니다."), SlotIndex);
		return false;
	}

	OutItem.ItemId = ItemId;
	OutItem.Quantity = 0;
	OutItem.ItemDataAsset = FindItemData(ItemId);

	// 인벤토리 보유 목록에서 현재 실제 보유 수량 갱신
	for (const FFTInventoryItem& Item : Items)
	{
		if (Item.ItemId == ItemId)
		{
			OutItem.Quantity = Item.Quantity;
			break;
		}
	}

	return true;
}

bool UFTInventoryComponent::GetInventoryItemAtIndex(int32 SlotIndex, FFTInventoryItem& OutItem) const
{
	if (!Items.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogFTItem, Warning, TEXT("GetInventoryItemAtIndex 호출 실패: 유효하지 않은 인벤토리 슬롯 인덱스 %d (현재 등록된 아이템 종류 수: %d)"), SlotIndex, Items.Num());
		return false;
	}

	OutItem = Items[SlotIndex];
	return true;
}

void UFTInventoryComponent::HandleItemPickedUpMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	AActor* Owner = GetOwner();

	// 메시지 발신 주체 혹은 타겟이 이 컴포넌트의 소유주(플레이어)인지 판별하여 본인 습득물만 추가
	if (Payload.TargetActor == Owner || Payload.InstigatorActor == Owner ||
		(Payload.TargetActor == nullptr && Payload.InstigatorActor == nullptr))
	{
		UE_LOG(LogFTItem, Log, TEXT("인벤토리 컴포넌트가 아이템 습득 메시지(Event.Item.PickedUp)를 수신했습니다. 대상 아이템: %s"), *Payload.ItemId.ToString());
		AddItem(Payload.ItemId, 1);
	}
}

void UFTInventoryComponent::HandleItemConsumedMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	AActor* Owner = GetOwner();

	// 메시지 발신 주체(소모한 액터)가 이 인벤토리의 소유주(플레이어)인지 확인
	if (Payload.InstigatorActor == Owner)
	{
		UE_LOG(LogFTItem, Log, TEXT("인벤토리 컴포넌트가 아이템 소모 메시지(Event.Item.Consumed)를 수신했습니다. 대상 아이템: %s"), *Payload.ItemId.ToString());
		
		// 인벤토리에서 아이템 1개 차감
		RemoveItem(Payload.ItemId, 1);
	}
}

TArray<FFTInventoryItem> UFTInventoryComponent::GetItemsByCategory(EFTItemCategoryType Category) const
{
	if (Category == EFTItemCategoryType::None)
	{
		return Items;
	}

	TArray<FFTInventoryItem> FilteredItems;
	for (const FFTInventoryItem& Item : Items)
	{
		if (Item.ItemDataAsset && Item.ItemDataAsset->ItemData.CategoryType == Category)
		{
			FilteredItems.Add(Item);
		}
	}
	return FilteredItems;
}

bool UFTInventoryComponent::RemoveItemsByIndices(const TArray<int32>& TargetIndices)
{
	if (TargetIndices.Num() == 0) return false;

	// 인덱스 정렬. 큰 인덱스(뒤쪽)부터 차례대로 지워야 앞쪽 인덱스 순서가 꼬이지 않는다.
	TArray<int32> SortedIndices = TargetIndices;
	SortedIndices.Sort([](const int32& A, const int32& B) { return A > B; });

	bool bChanged = false;
	for (int32 Index : SortedIndices)
	{
		if (Items.IsValidIndex(Index))
		{
			Items.RemoveAt(Index);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		UpdateWeight();
		OnInventoryChanged.Broadcast();
	}

	return bChanged;
}

void UFTInventoryComponent::RequestDropItems(const TArray<int32>& TargetIndices)
{
	AActor* Owner = GetOwner();
	if (!Owner || TargetIndices.Num() == 0) return;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

	for (int32 Index : TargetIndices)
	{
		if (Items.IsValidIndex(Index))
		{
			const FFTInventoryItem& TargetItem = Items[Index];

			// 드롭할 정보 페이로드 작성
			FFTMessagePayloadStruct Payload;
			Payload.InstigatorActor = Owner;
			Payload.ItemId = TargetItem.ItemId;
			Payload.Value = static_cast<float>(TargetItem.Quantity); // 수량 전달

			// 드롭 스폰 담당 시스템이 수신할 수 있도록 메시지 발행
			MessageSubsystem.BroadcastMessage(TAG_FT_Request_DropItem, Payload);
		}
	}
}
