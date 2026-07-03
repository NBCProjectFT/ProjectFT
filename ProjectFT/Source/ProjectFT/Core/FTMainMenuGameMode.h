#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FTMainMenuGameMode.generated.h"

/**
 * Main menu only GameMode.
 * Keeps menu UI setup out of gameplay maps so player pawn/input rules stay owned by AFTGameMode.
 */
UCLASS()
class PROJECTFT_API AFTMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;
};
