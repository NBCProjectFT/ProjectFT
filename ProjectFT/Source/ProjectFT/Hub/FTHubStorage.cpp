#include "FTHubStorage.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/UI/HubUI//FTHubStorageWidget.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/ViewModel/FTHubStorageViewModel.h"

AFTHubStorage::AFTHubStorage()
	:HubStorageWidget(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
	StorageInventory = CreateDefaultSubobject<UFTInventoryComponent>(TEXT("StorageInventory"));
	TestStorageItems.Add({ TEXT("ID_Healing_Water"), 1 });
	TestStorageItems.Add({ TEXT("ID_Coin"), 1000 });
}

void AFTHubStorage::BeginPlay()
{
	Super::BeginPlay();

	
	
	for (const FTStorageItemStruct& TestItem : TestStorageItems)
	{
		if (!TestItem.ItemID.IsNone() && TestItem.Count > 0)
		{
			AddStorageItem(TestItem.ItemID, TestItem.Count);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Before Storage Test"));
	PrintStorageItems();
}

bool AFTHubStorage::AddStorageItem(FName ItemID, int32 Count)
{
	return StorageInventory
		? StorageInventory->AddItem(ItemID, Count)
		: false;
}

bool AFTHubStorage::RemoveStorageItem(FName ItemID, int32 Count)
{
	if (!StorageInventory)
	{
		return false;
	}

	return StorageInventory->RemoveItem(ItemID, Count);
	
}

int32 AFTHubStorage::GetStorageItemCount(FName ItemID) const
{
	return StorageInventory
		? StorageInventory->GetItemQuantity(ItemID)
		: 0;
}

const TArray<FTStorageItemStruct>& AFTHubStorage::GetStorageItems() const
{
	CachedStorageItems.Reset();

	if (!StorageInventory)
	{
		return CachedStorageItems;
	}
	
	for (const FFTInventoryItem& Item : StorageInventory->GetItems())
	{
		CachedStorageItems.Add({ Item.ItemId, Item.Quantity });
	}

	return CachedStorageItems;
}

bool AFTHubStorage::StoreItemFromInventory(UFTInventoryComponent* SourceInventory, FName ItemID, int32 Count)
{
	if (!SourceInventory || !StorageInventory || ItemID.IsNone() || Count <= 0)
	{
		return false;
	}

	if (SourceInventory->GetItemQuantity(ItemID) < Count)
	{
		UE_LOG(LogTemp, Warning, TEXT("Store Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!AddStorageItem(ItemID, Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Store Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!SourceInventory->RemoveItem(ItemID, Count))
	{
		RemoveStorageItem(ItemID, Count);
		UE_LOG(LogTemp, Warning, TEXT("Store Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Store Success: %s x%d"), *ItemID.ToString(), Count);
	return true;
}

bool AFTHubStorage::TakeItemToInventory(UFTInventoryComponent* TargetInventory, FName ItemID, int32 Count)
{
	if (!TargetInventory || !StorageInventory || ItemID.IsNone() || Count <= 0)
	{
		return false;
	}

	if (GetStorageItemCount(ItemID) < Count)
	{
		UE_LOG(LogTemp, Warning, TEXT("Take Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!TargetInventory->AddItem(ItemID, Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Take Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	if (!RemoveStorageItem(ItemID, Count))
	{
		TargetInventory->RemoveItem(ItemID, Count);
		UE_LOG(LogTemp, Warning, TEXT("Take Failed: %s x%d"), *ItemID.ToString(), Count);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Take Success: %s x%d"), *ItemID.ToString(), Count);
	return true;
}

void AFTHubStorage::PrintStorageItems() const
{
	const TArray<FTStorageItemStruct>& StorageItems = GetStorageItems();

	if (StorageItems.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Storage Empty"));
		return;
	}

	for (const FTStorageItemStruct& StorageItem : StorageItems)
	{
		UE_LOG(LogTemp, Warning, TEXT("Storage: %s x%d"),
			*StorageItem.ItemID.ToString(),
			StorageItem.Count
		);
	}
}

UFTInventoryComponent* AFTHubStorage::GetStorageInventory() const
{
	return StorageInventory;
}

bool AFTHubStorage::Interact_Implementation(AActor* Interactor)
{
	OpenStorageWidget(Interactor);
	return true;
}

FText AFTHubStorage::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("창고 열기"));
}

void AFTHubStorage::OpenStorageWidget(AActor* Interactor)
{
	if (!HubStorageWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("HubStorageWidgetClass is not assigned."));
		return;
	}
	
	APlayerController* PlayerController = nullptr;

	if (APawn* InteractorPawn = Cast<APawn>(Interactor))
	{
		PlayerController = Cast<APlayerController>(InteractorPawn->GetController());
	}
	
	if (HubStorageWidget && HubStorageWidget->IsInViewport())
	{
		CloseStorageWidget();
		return;
	}

	

	

	if (!PlayerController)
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	if (!PlayerController)
	{
		return;
	}

	UFTInventoryComponent* PlayerInventory = nullptr;
	if (Interactor)
	{
		PlayerInventory = Interactor->FindComponentByClass<UFTInventoryComponent>();
	}

	if (!PlayerInventory && PlayerController->GetPawn())
	{
		PlayerInventory = PlayerController->GetPawn()->FindComponentByClass<UFTInventoryComponent>();
	}

	if (!PlayerInventory)
	{
		PlayerInventory = PlayerController->FindComponentByClass<UFTInventoryComponent>();
	}

	if (!HubStorageWidget)
	{
		HubStorageWidget = CreateWidget<UFTHubStorageWidget>(PlayerController, HubStorageWidgetClass);
		if (!HubStorageWidget)
		{
			return;
		}
	}

	if (!HubStorageViewModel)
	{
		HubStorageViewModel = NewObject<UFTHubStorageViewModel>(this);
	}

	HubStorageWidget->InitializeStorageWidget(this, PlayerInventory, HubStorageViewModel);

	if (!HubStorageWidget->IsInViewport())
	{
		HubStorageWidget->AddToViewport();

		PlayerController->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(HubStorageWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}
}

void AFTHubStorage::CloseStorageWidget()
{
	if (HubStorageWidget && HubStorageWidget->IsInViewport())
	{
		HubStorageWidget->RemoveFromParent();
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
}
