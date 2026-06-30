#include "FTLoadingGameMode.h"

#include "FTLogChannels.h"
#include "../Data/FTGameDataAsset.h"
#include "../Manager/AssetManager/FTAssetManager.h"
#include "../UI/FTLoadingWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AFTLoadingGameMode::AFTLoadingGameMode()
{
	MainLevelName = TEXT("Lvl_Main");
}

void AFTLoadingGameMode::StartPlay()
{
	Super::StartPlay();

	CreateLoadingWidget();

	UE_LOG(LogFTFlow, Log, TEXT("Loading sequence started."));
	UFTAssetManager::Get().PreloadGameDataAssetsAsync(
		FSimpleDelegate::CreateUObject(this, &AFTLoadingGameMode::HandlePreloadCompleted),
		FFTAssetLoadProgressDelegate::CreateUObject(this, &AFTLoadingGameMode::HandleLoadProgress)
	);
}

void AFTLoadingGameMode::CreateLoadingWidget()
{
	const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData();
	if (!GameData)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Loading widget was not created because game data is missing."));
		return;
	}

	TSubclassOf<UFTLoadingWidget> LoadingWidgetClass = UFTAssetManager::GetSubclass(GameData->LoadingWidgetClass);
	if (!LoadingWidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("LoadingWidgetClass is not set."));
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
	UE_LOG(LogFTFlow, Log, TEXT("Loading sequence completed. Ready to open %s."), *MainLevelName.ToString());

	if (LoadingWidget)
	{
		LoadingWidget->ReadyToStart();
		if (LoadingWidget->BindOnButtonClicked([this]()
		{
			UGameplayStatics::OpenLevel(this, MainLevelName);
		}))
		{
			return;
		}
	}

	UGameplayStatics::OpenLevel(this, MainLevelName);
}
