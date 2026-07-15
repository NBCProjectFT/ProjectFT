#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FTLoadingGameMode.generated.h"

class UFTLoadingWidget;
class UFTLevelPreloadDataAsset;

UCLASS()
class PROJECTFT_API AFTLoadingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFTLoadingGameMode();

	virtual void StartPlay() override;

private:
	void CreateLoadingWidget();
	void HandleLoadProgress(const FString& AssetName, int32 CompletedCount, int32 TotalCount);
	void HandlePreloadCompleted();
	void NotifyLoadingConfirmed();

private:
	UPROPERTY(Transient)
	TObjectPtr<UFTLoadingWidget> LoadingWidget;
};
