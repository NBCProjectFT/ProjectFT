#pragma once

#include "CoreMinimal.h"
#include "FTPauseMenuConfirmType.generated.h"

UENUM(BlueprintType)
enum class EFTPauseMenuConfirmType : uint8
{
	None,
	MainMenu,
	ReturnToBase,
	QuitGame
};
