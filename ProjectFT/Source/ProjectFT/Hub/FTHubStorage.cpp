#include "FTHubStorage.h"
#include "FTHubActorUtils.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

AFTHubStorage::AFTHubStorage()
{
	PrimaryActorTick.bCanEverTick = false;
	StorageInventory = CreateDefaultSubobject<UFTInventoryComponent>(TEXT("StorageInventory"));
	TestStorageItems.Add({ TEXT("ID_Healing_Water"), 1 });
	TestStorageItems.Add({ TEXT("ID_Coin"), 1000 });
}

void AFTHubStorage::BeginPlay()
{
	Super::BeginPlay();

	if (UFTStorageSubsystem* StorageSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFTStorageSubsystem>() : nullptr)
	{
		// 액터는 초기 아이템 목록을 넘기기만 하고, 실제 추가 규칙은 StorageSubsystem이 처리한다.
		StorageSubsystem->InitializeStorage(StorageInventory, TestStorageItems);
		UE_LOG(LogTemp, Warning, TEXT("Before Storage Test"));
		StorageSubsystem->PrintStorageItems(StorageInventory);
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
	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		// Interactor에서 플레이어 인벤토리를 찾아 UIManager에 넘기면 Widget/ViewModel이 이후 흐름을 맡는다.
		UIManager->ShowStorage(this, FTHubActorUtils::FindPlayerInventory(this, Interactor));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Storage widget was not opened because UIManager is missing."));
}

void AFTHubStorage::CloseStorageWidget()
{
	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->HideStorage();
	}
}
