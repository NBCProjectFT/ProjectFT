#include "FTHubStorage.h"
#include "FTHubActorUtils.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTStorageSubsystem.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

AFTHubStorage::AFTHubStorage()
{
	PrimaryActorTick.bCanEverTick = false;
	StorageInventory = CreateDefaultSubobject<UFTInventoryComponent>(TEXT("StorageInventory"));
}

void AFTHubStorage::BeginPlay()
{
	Super::BeginPlay();

	if (UFTStorageSubsystem* StorageSubsystem = GetGameInstance()->GetSubsystem<UFTStorageSubsystem>())
	{
		StorageSubsystem->InitializeStorage(StorageInventory, InitialItems);
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
