#include "FTMainMenuGameMode.h"

#include "FTLogChannels.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

void AFTMainMenuGameMode::StartPlay()
{
	Super::StartPlay();

	UE_LOG(LogFTFlow, Log, TEXT("MainMenuGameMode StartPlay. Map=%s GameMode=%s"),
		GetWorld() ? *GetWorld()->GetMapName() : TEXT("None"),
		*GetClass()->GetName());

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->ShowMainMenu();
		}
	}
}
