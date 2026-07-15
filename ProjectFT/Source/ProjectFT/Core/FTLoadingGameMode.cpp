#include "FTLoadingGameMode.h"

#include "FTGameFlowSubsystem.h"
#include "FTLogChannels.h"
#include "../Manager/AssetManager/FTAssetManager.h"
#include "../UI/FTLoadingWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AFTLoadingGameMode::AFTLoadingGameMode()
{
}

void AFTLoadingGameMode::StartPlay()
{
	Super::StartPlay();

	CreateLoadingWidget();

	UE_LOG(LogFTFlow, Log, TEXT("Loading sequence started."));
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* FlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			const TSoftObjectPtr<UFTLevelPreloadDataAsset> LevelPreloadDataAsset = FlowSubsystem->GetCurrentStateLevelPreloadDataAsset();
			UE_LOG(LogFTFlow, Log, TEXT("LoadingGameMode selected preload data. State=%d DataAsset=%s"),
				static_cast<uint8>(FlowSubsystem->GetCurrentFlowState()),
				*LevelPreloadDataAsset.ToSoftObjectPath().ToString());

			UFTAssetManager::Get().PreloadLevelAssetsAsync(
				LevelPreloadDataAsset,
				FSimpleDelegate::CreateUObject(this, &AFTLoadingGameMode::HandlePreloadCompleted),
				FFTAssetLoadProgressDelegate::CreateUObject(this, &AFTLoadingGameMode::HandleLoadProgress)
			);
			return;
		}
	}

	UE_LOG(LogFTFlow, Error, TEXT("Loading sequence has no FlowSubsystem. Completing without preload."));
	HandlePreloadCompleted();
}

void AFTLoadingGameMode::CreateLoadingWidget()
{
	TSubclassOf<UFTLoadingWidget> LoadingWidgetClass = UFTAssetManager::Get().GetLoadingWidgetClass();
	if (!LoadingWidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("LoadingWidgetClass is not set in UI data."));
		return;
	}

	LoadingWidget = CreateWidget<UFTLoadingWidget>(GetWorld(), LoadingWidgetClass);
	if (!LoadingWidget)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Failed to create loading widget."));
		return;
	}

	LoadingWidget->AddToViewport();
	LoadingWidget->SetPercent(0.0f);
	LoadingWidget->SetObjectName(TEXT("Game Data"), 0, 1);

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(LoadingWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}

	UE_LOG(LogFTUI, Log, TEXT("Loading widget created: %s"), *GetNameSafe(LoadingWidget));
}

void AFTLoadingGameMode::HandleLoadProgress(const FString& AssetName, int32 CompletedCount, int32 TotalCount)
{
	if (!LoadingWidget || TotalCount <= 0)
	{
		return;
	}

	LoadingWidget->SetObjectName(AssetName, CompletedCount, TotalCount);
	LoadingWidget->SetPercent(static_cast<float>(CompletedCount) / static_cast<float>(TotalCount));
	UE_LOG(LogFTUI, Log, TEXT("Loading widget progress: %s (%d / %d)"), *AssetName, CompletedCount, TotalCount);
}

void AFTLoadingGameMode::HandlePreloadCompleted()
{
	UE_LOG(LogFTFlow, Log, TEXT("Loading sequence completed. Waiting for player confirmation."));

	if (LoadingWidget)
	{
		LoadingWidget->ReadyToStart();
		if (LoadingWidget->BindOnButtonClicked([this]()
		{
			NotifyLoadingConfirmed();
		}))
		{
			return;
		}
	}

	NotifyLoadingConfirmed();
}

void AFTLoadingGameMode::NotifyLoadingConfirmed()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* FlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			FlowSubsystem->CompleteLoadingAndOpenCurrentStateLevel();
			return;
		}
	}

	UE_LOG(LogFTFlow, Error, TEXT("Loading confirmation ignored because FlowSubsystem is missing."));
}
