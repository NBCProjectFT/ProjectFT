#include "FTHubTerminal.h"

#include "FTHubActorUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/Core/FTSaveSubsystem.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
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
	if (bIsCameraTransitionInProgress)
	{
		return;
	}

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
	// 진입 또는 복귀 카메라 보간이 끝나기 전에는 열기·닫기 입력을 모두 무시한다.
	if (bIsCameraTransitionInProgress)
	{
		return;
	}

	if (bIsInComputerUseMode)
	{
		CloseHubWidget();
		return;
	}

	ConfigureObjectiveSubsystem();

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
	bIsCameraTransitionInProgress = false;

	AActor* Interactor = PendingInteractor.Get();
	PendingInteractor = nullptr;

	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->ShowHubMain(this, HubStorage, FTHubActorUtils::FindPlayerInventory(this, Interactor));
		if (UIManager->IsHubMainOpen())
		{
			// 허브 메인 화면이 실제로 열린 경우에만 컴퓨터 접속 퀘스트를 진행한다.
			FFTMessagePayloadStruct Payload;
			Payload.InstigatorActor = Interactor;
			UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_HubComputerAccessed, Payload);
		}
		else
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
	bIsCameraTransitionInProgress = CameraBlendTime > KINDA_SMALL_NUMBER;

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
			bIsCameraTransitionInProgress = CameraBlendTime > KINDA_SMALL_NUMBER;
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

	if (bIsCameraTransitionInProgress)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				ComputerExitTransitionTimerHandle,
				this,
				&AFTHubTerminal::FinishComputerExitTransition,
				CameraBlendTime,
				false);
		}
		else
		{
			bIsCameraTransitionInProgress = false;
		}
	}
}

void AFTHubTerminal::FinishComputerExitTransition()
{
	bIsCameraTransitionInProgress = false;
}

void AFTHubTerminal::ConfigureObjectiveSubsystem()
{
	UGameInstance* GameInstance = GetGameInstance();
	UFTObjectiveSubsystem* ObjectiveSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTObjectiveSubsystem>() : nullptr;
	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->ConfigureHubQuests(QuestDataTable, HubStorage, InitialQuestIDs);
	}

	if (UFTSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTSaveSubsystem>() : nullptr)
	{
		SaveSubsystem->RestoreQuestState();
	}

	// 저장 데이터가 있으면 그 상태를 우선 복원한 뒤, 아직 시작하지 않은 자동 수락 퀘스트만 활성화한다.
	if (ObjectiveSubsystem)
	{
		for (const FName QuestID : AutoAcceptedQuestIDs)
		{
			if (QuestID.IsNone()
				|| ObjectiveSubsystem->IsQuestActive(QuestID)
				|| ObjectiveSubsystem->IsQuestCompleted(QuestID))
			{
				continue;
			}

			ObjectiveSubsystem->UnlockQuest(QuestID);
			ObjectiveSubsystem->AcceptQuest(QuestID);
		}
	}

	UFTShopSubsystem* ShopSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTShopSubsystem>() : nullptr;
	if (ShopSubsystem)
	{
		ShopSubsystem->ConfigureHubStorage(HubStorage);
	}
}
