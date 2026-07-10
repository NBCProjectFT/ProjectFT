#include "FTHubTerminal.h"

#include "FTHubActorUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

AFTHubTerminal::AFTHubTerminal()
	: QuestDataTable(nullptr)
	, HubStorage(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TerminalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TerminalCamera"));
	TerminalCamera->SetupAttachment(SceneRoot);
	TerminalCamera->SetRelativeLocation(FVector(-120.0f, 0.0f, 70.0f));
	TerminalCamera->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
}

void AFTHubTerminal::BeginPlay()
{
	Super::BeginPlay();
	ConfigureObjectiveSubsystem();
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShowHubWidgetTimerHandle);
	}
	PendingInteractor = nullptr;

	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->HideHubMain();
	}

	ExitComputerUseMode();
}

void AFTHubTerminal::OpenHubWidget(AActor* Interactor)
{
	ConfigureObjectiveSubsystem();

	if (bIsInComputerUseMode)
	{
		CloseHubWidget();
		return;
	}

	EnterComputerUseMode(Interactor);
	if (!bIsInComputerUseMode)
	{
		return;
	}

	PendingInteractor = Interactor;
	if (UWorld* World = GetWorld(); World && CameraBlendTime > KINDA_SMALL_NUMBER)
	{
		World->GetTimerManager().SetTimer(
			ShowHubWidgetTimerHandle,
			this,
			&AFTHubTerminal::ShowHubWidgetAfterCameraBlend,
			CameraBlendTime,
			false);
	}
	else
	{
		ShowHubWidgetAfterCameraBlend();
	}
}

void AFTHubTerminal::ShowHubWidgetAfterCameraBlend()
{
	AActor* Interactor = PendingInteractor.Get();
	PendingInteractor = nullptr;

	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->ShowHubMain(this, HubStorage, FTHubActorUtils::FindPlayerInventory(this, Interactor));
		if (!UIManager->IsHubMainOpen())
		{
			ExitComputerUseMode();
		}
	}
	else
	{
		ExitComputerUseMode();
	}
}

void AFTHubTerminal::EnterComputerUseMode(AActor* Interactor)
{
	if (bIsInComputerUseMode)
	{
		return;
	}

	APawn* InteractingPawn = Cast<APawn>(Interactor);
	APlayerController* PlayerController = InteractingPawn
		? Cast<APlayerController>(InteractingPawn->GetController())
		: GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	UsingPlayerController = PlayerController;
	UsingPawn = PlayerController->GetPawn();
	PreviousViewTarget = PlayerController->GetViewTarget();
	bIsInComputerUseMode = true;

	if (bLockPlayerMovementDuringUse)
	{
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);

		if (ACharacter* Character = Cast<ACharacter>(UsingPawn))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->StopMovementImmediately();
				Movement->DisableMovement();
			}
		}
	}

	PlayerController->SetViewTargetWithBlend(this, CameraBlendTime);
}

void AFTHubTerminal::ExitComputerUseMode()
{
	if (!bIsInComputerUseMode)
	{
		return;
	}

	if (UsingPlayerController)
	{
		if (PreviousViewTarget)
		{
			UsingPlayerController->SetViewTargetWithBlend(PreviousViewTarget, CameraBlendTime);
		}

		if (bLockPlayerMovementDuringUse)
		{
			UsingPlayerController->SetIgnoreMoveInput(false);
			UsingPlayerController->SetIgnoreLookInput(false);

			if (ACharacter* Character = Cast<ACharacter>(UsingPawn))
			{
				if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
				{
					Movement->SetMovementMode(MOVE_Walking);
				}
			}
		}
	}

	UsingPlayerController = nullptr;
	UsingPawn = nullptr;
	PreviousViewTarget = nullptr;
	bIsInComputerUseMode = false;
}

void AFTHubTerminal::ConfigureObjectiveSubsystem()
{
	UGameInstance* GameInstance = GetGameInstance();
	UFTObjectiveSubsystem* ObjectiveSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTObjectiveSubsystem>() : nullptr;
	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->ConfigureHubQuests(QuestDataTable, HubStorage, InitialQuestIDs);
	}

	UFTShopSubsystem* ShopSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTShopSubsystem>() : nullptr;
	if (ShopSubsystem)
	{
		ShopSubsystem->ConfigureHubStorage(HubStorage);
	}
}
