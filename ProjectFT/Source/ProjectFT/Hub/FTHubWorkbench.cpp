#include "FTHubWorkbench.h"

#include "FTHubActorUtils.h"
#include "FTHubStorage.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

AFTHubWorkbench::AFTHubWorkbench()
	: HubStorage(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AFTHubWorkbench::Interact_Implementation(AActor* Interactor)
{
	OpenCraftWidget(Interactor);

	return true;
}

FText AFTHubWorkbench::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("작업대 사용"));
}

void AFTHubWorkbench::OpenCraftWidget(AActor* Interactor)
{
	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->ShowCrafting(
			FTHubActorUtils::FindPlayerInventory(this, Interactor),
			GetStorageInventory());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Craft widget was not opened because UIManager is missing."));
}

UFTInventoryComponent* AFTHubWorkbench::GetStorageInventory() const
{
	return HubStorage ? HubStorage->GetStorageInventory() : nullptr;
}
