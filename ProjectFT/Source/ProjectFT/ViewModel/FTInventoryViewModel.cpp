#include "FTInventoryViewModel.h"

UFTInventoryViewModel::UFTInventoryViewModel()
{
	LinkedInventory = nullptr;
	InventoryItems.Empty();
	CurrentWeight = 0.0f;
	MaxWeight = 40.0f; // 기본 무게 한도
	SelectedItem = NAME_None;
	SelectedItemIndex = INDEX_NONE;
	CurrentCategory = EFTItemCategoryType::None;
}

void UFTInventoryViewModel::Initialize(UFTInventoryComponent* InInventoryComponent)
{
	if (!InInventoryComponent) return;

	// 이전에 다른 인벤토리에 바인딩돼 있었다면 정리(폰 교체/리스폰 시 스테일 콜백 방지)
	if (UFTInventoryComponent* Prev = LinkedInventory.Get())
	{
		if (Prev != InInventoryComponent)
		{
			Prev->OnInventoryChanged.RemoveDynamic(this, &UFTInventoryViewModel::NotifyChanged);
		}
	}

	LinkedInventory = InInventoryComponent;

	// 인벤토리 변경 시 뷰모델이 감지하여 자동 동기화하도록 델리게이트 바인딩(중복 바인딩 방지)
	InInventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UFTInventoryViewModel::NotifyChanged);

	// 초기 상태 갱신
	NotifyChanged();
}

void UFTInventoryViewModel::NotifyChanged()
{
	UFTInventoryComponent* Inventory = LinkedInventory.Get();
	if (!Inventory) return;

	// 1. 무게 데이터 동기화
	CurrentWeight = Inventory->GetCurrentWeight();
	MaxWeight = Inventory->GetMaxWeight();

	// 2. 현재 선택된 상단 카테고리 탭에 따라 슬롯 아이템 ID 리스트 필터링
	InventoryItems.Empty();
	TArray<FFTInventoryItem> FilteredItems = Inventory->GetItemsByCategory(CurrentCategory);
	for (const FFTInventoryItem& Item : FilteredItems)
	{
		InventoryItems.Add(Item.ItemId);
	}

	// 3. 우측 상세 패널 정보 동기화
	if (SelectedItemIndex != INDEX_NONE)
	{
		TArray<FFTInventoryItem> CurrentCategoryItems = Inventory->GetItemsByCategory(CurrentCategory);
		if (CurrentCategoryItems.IsValidIndex(SelectedItemIndex))
		{
			SelectedItemDetail = CurrentCategoryItems[SelectedItemIndex];
			SelectedItem = SelectedItemDetail.ItemId;
		}
		else
		{
			// 소모 등으로 유효 인덱스가 무너졌을 경우 선택 비우기
			SelectedItemDetail = FFTInventoryItem();
			SelectedItem = NAME_None;
			SelectedItemIndex = INDEX_NONE;
		}
	}
	else
	{
		SelectedItemDetail = FFTInventoryItem();
		SelectedItem = NAME_None;
	}

	// 4. 아이템 삭제 및 소진으로 인해 유효하지 않게 된 다중 체크 인덱스 소거
	TArray<FFTInventoryItem> FullItems = Inventory->GetItems();
	TArray<int32> InvalidIndices;
	for (int32 Index : SelectedIndices)
	{
		if (!FullItems.IsValidIndex(Index))
		{
			InvalidIndices.Add(Index);
		}
	}
	for (int32 Index : InvalidIndices)
	{
		SelectedIndices.Remove(Index);
	}

	// 5. UI 변경 공지 전송
	OnViewModelChanged.Broadcast();
}

void UFTInventoryViewModel::SetCategoryTab(EFTItemCategoryType NewCategory)
{
	if (CurrentCategory == NewCategory) return;

	CurrentCategory = NewCategory;

	// 카테고리가 전환되면 우측 상세 패널 선택만 초기화 (다중 체크 박스 상태는 유지)
	SelectedItemIndex = INDEX_NONE;
	SelectedItemDetail = FFTInventoryItem();
	SelectedItem = NAME_None;

	NotifyChanged();
}

void UFTInventoryViewModel::SelectItemDetailAtIndex(int32 SlotIndex)
{
	SelectedItemIndex = SlotIndex;
	NotifyChanged();
}

bool UFTInventoryViewModel::RegisterSelectedToQuickSlot(int32 SlotIndex)
{
	UFTInventoryComponent* Inventory = LinkedInventory.Get();
	if (!Inventory || SelectedItem.IsNone())
	{
		return false;
	}

	// SlotIndex는 대상 퀵슬롯 번호일 뿐이고, 등록되는 아이템은 현재 UI에서 선택된 것(SelectedItem)이다.
	// (SetQuickSlot이 미보유/Common 등은 자체적으로 거부하고 OnInventoryChanged를 브로드캐스트해 UI를 갱신한다.)
	return Inventory->SetQuickSlot(SlotIndex, SelectedItem);
}

void UFTInventoryViewModel::SetItemSelection(int32 SlotIndex, bool bIsSelected)
{
	UFTInventoryComponent* Inventory = LinkedInventory.Get();
	if (!Inventory) return;

	// 1. 현재 카테고리 필터링이 씌워진 아이템 배열
	TArray<FFTInventoryItem> FilteredItems = Inventory->GetItemsByCategory(CurrentCategory);
	if (!FilteredItems.IsValidIndex(SlotIndex)) return;

	FName TargetId = FilteredItems[SlotIndex].ItemId;

	// 2. 인벤토리 오리지널 전체 배열에서 고유 인덱스를 역추적하여 탐색
	TArray<FFTInventoryItem> FullItems = Inventory->GetItems();
	int32 RealIndex = INDEX_NONE;
	for (int32 i = 0; i < FullItems.Num(); ++i)
	{
		if (FullItems[i].ItemId == TargetId)
		{
			RealIndex = i;
			break;
		}
	}

	if (RealIndex == INDEX_NONE) return;

	// 3. 탐색된 인덱스를 체크 셋에 추가/삭제
	if (bIsSelected)
	{
		SelectedIndices.Add(RealIndex);
	}
	else
	{
		SelectedIndices.Remove(RealIndex);
	}

	OnViewModelChanged.Broadcast();
}

void UFTInventoryViewModel::ClearSelection()
{
	SelectedIndices.Empty();
	SelectedItemIndex = INDEX_NONE;
	SelectedItemDetail = FFTInventoryItem();
	SelectedItem = NAME_None;

	OnViewModelChanged.Broadcast();
}

void UFTInventoryViewModel::DiscardSelectedItems()
{
	UFTInventoryComponent* Inventory = LinkedInventory.Get();
	if (!Inventory || SelectedIndices.Num() == 0) return;

	TArray<int32> IndicesArray = SelectedIndices.Array();

	// 1. 바닥에 스폰해 달라고 드롭 요청 메시지 발행
	Inventory->RequestDropItems(IndicesArray);

	// 2. 인벤토리에서 실제 메모리 제거
	Inventory->RemoveItemsByIndices(IndicesArray);

	// 3. 선택 갱신 및 UI 리프레시
	ClearSelection();
}

FLinearColor UFTInventoryViewModel::GetWeightBarColor() const
{
	const float Percent = GetWeightPercent();

	if (Percent >= 0.9f)
	{
		return FLinearColor::Red; // 90% 이상 위험
	}
	else if (Percent >= 0.7f)
	{
		return FLinearColor::Yellow; // 70% 이상 경고
	}
	return FLinearColor::Green; // 안정 상태
}

FText UFTInventoryViewModel::GetWeightText() const
{
	FString FormatString = FString::Printf(TEXT("%.1f / %.1f kg"), CurrentWeight, MaxWeight);
	return FText::FromString(FormatString);
}

float UFTInventoryViewModel::GetWeightPercent() const
{
	if (MaxWeight <= 0.0f) return 0.0f;
	return FMath::Clamp(CurrentWeight / MaxWeight, 0.0f, 1.0f);
}

bool UFTInventoryViewModel::IsIndexSelected(int32 SlotIndex) const
{
	UFTInventoryComponent* Inventory = LinkedInventory.Get();
	if (!Inventory)
	{
		return false;
	}

	// 1. 현재 카테고리 필터링이 씌워진 아이템 배열
	TArray<FFTInventoryItem> FilteredItems = Inventory->GetItemsByCategory(CurrentCategory);
	if (!FilteredItems.IsValidIndex(SlotIndex))
	{
		return false;
	}

	FName TargetId = FilteredItems[SlotIndex].ItemId;

	// 2. 인벤토리 오리지널 전체 배열에서 고유 인덱스를 역추적하여 탐색
	TArray<FFTInventoryItem> FullItems = Inventory->GetItems();
	for (int32 i = 0; i < FullItems.Num(); ++i)
	{
		if (FullItems[i].ItemId == TargetId)
		{
			return SelectedIndices.Contains(i);
		}
	}

	return false;
}
