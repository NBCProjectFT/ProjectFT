#include "FTInventoryWidget.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTInventoryViewModel.h"
#include "ProjectFT/Components/FTInventoryComponent.h"

void UFTInventoryWidget::NativeConstruct()
{
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		UFTUIManagerSubsystem* UIManager = GI->GetSubsystem<UFTUIManagerSubsystem>();
		if (UIManager)
		{
			ViewModel = UIManager->InventoryViewModel;
			if (ViewModel)
			{
				ViewModel->OnViewModelChanged.AddUniqueDynamic(this, &UFTInventoryWidget::HandleViewModelChanged);
			}
		}
	}

	Super::NativeConstruct();

	bHasConstructed = true;
	
	// 이미 뷰모델이 초기화되어 있다면 즉시 동기화
	HandleViewModelChanged();
}

void UFTInventoryWidget::SetupInventory(UFTInventoryComponent* InInventoryComponent)
{
	if (!InInventoryComponent) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UFTUIManagerSubsystem* UIManager = GI->GetSubsystem<UFTUIManagerSubsystem>();
	if (!UIManager) return;

	ViewModel = UIManager->InventoryViewModel;
	if (!ViewModel) return;

	// 뷰모델을 타겟 인벤토리 컴포넌트로 초기화
	ViewModel->Initialize(InInventoryComponent);
	
	// 인벤토리가 열리거나 새로 세팅될 때 체크박스 선택 초기화
	ViewModel->ClearSelection();
	
	ViewModel->OnViewModelChanged.AddUniqueDynamic(this, &UFTInventoryWidget::HandleViewModelChanged);
	
	// 초기 동기화 호출
	HandleViewModelChanged();
}

void UFTInventoryWidget::HandleViewModelChanged()
{
	if (!bHasConstructed || !ViewModel) return;

	// UI 갱신 이벤트 호출 (블루프린트에서 오버라이드하여 처리 가능)
	UpdateWeight(ViewModel->CurrentWeight, ViewModel->MaxWeight);
	RefreshItemList();
	ShowItemDetail(ViewModel->SelectedItem);
}
