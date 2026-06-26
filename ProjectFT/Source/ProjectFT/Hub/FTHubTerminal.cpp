#include "FTHubTerminal.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/UI/HubUI/FTHubMainWidget.h"

AFTHubTerminal::AFTHubTerminal()
	: HubQuestBoard(nullptr)
	, HubShop(nullptr)
	, HubMainWidget(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AFTHubTerminal::Interact_Implementation(AActor* Interactor)
{
	OpenHubWidget(Interactor);
	return true;
}

FText AFTHubTerminal::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("거점 메뉴 보기"));
}

void AFTHubTerminal::CloseHubWidget()
{
	if (HubMainWidget && HubMainWidget->IsInViewport())
	{
		HubMainWidget->RemoveFromParent();
	}

	APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;

	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
}

void AFTHubTerminal::OpenHubWidget(AActor* Interactor)
{
	if (HubMainWidget && HubMainWidget->IsInViewport())
	{
		CloseHubWidget();
		return;
	}

	if (!HubMainWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("HubMainWidgetClass is not assigned."));
		return;
	}

	APlayerController* PlayerController = nullptr;

	if (APawn* InteractorPawn = Cast<APawn>(Interactor))
	{
		PlayerController = Cast<APlayerController>(InteractorPawn->GetController());
	}

	if (!PlayerController)
	{
		PlayerController = GetWorld()
			? GetWorld()->GetFirstPlayerController()
			: nullptr;
	}

	if (!PlayerController)
	{
		return;
	}

	if (!HubMainWidget)
	{
		HubMainWidget = CreateWidget<UFTHubMainWidget>(
			PlayerController,
			HubMainWidgetClass
		);

		if (!HubMainWidget)
		{
			return;
		}
	}

	HubMainWidget->InitializeHubMain(
		this,
		HubQuestBoard,
		HubShop,
		FindPlayerInventory(Interactor)
	);

	if (!HubMainWidget->IsInViewport())
	{
		HubMainWidget->AddToViewport();

		PlayerController->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(HubMainWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}
}

UFTInventoryComponent* AFTHubTerminal::FindPlayerInventory(AActor* Interactor) const
{
	if (Interactor)
	{
		if (UFTInventoryComponent* PlayerInventory = Interactor->FindComponentByClass<UFTInventoryComponent>())
		{
			return PlayerInventory;
		}
	}

	const APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;

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
