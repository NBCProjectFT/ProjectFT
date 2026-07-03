#include "FTHubActorUtils.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

UFTInventoryComponent* FTHubActorUtils::FindPlayerInventory(const AActor* ContextActor, AActor* Interactor)
{
	if (Interactor)
	{
		if (UFTInventoryComponent* PlayerInventory = Interactor->FindComponentByClass<UFTInventoryComponent>())
		{
			return PlayerInventory;
		}
	}

	UWorld* World = ContextActor ? ContextActor->GetWorld() : nullptr;
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return nullptr;
	}

	if (APawn* Pawn = PlayerController->GetPawn())
	{
		if (UFTInventoryComponent* PlayerInventory = Pawn->FindComponentByClass<UFTInventoryComponent>())
		{
			return PlayerInventory;
		}
	}

	return PlayerController->FindComponentByClass<UFTInventoryComponent>();
}

UFTUIManagerSubsystem* FTHubActorUtils::GetUIManager(const AActor* ContextActor)
{
	UGameInstance* GameInstance = ContextActor ? ContextActor->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFTUIManagerSubsystem>() : nullptr;
}
