#pragma once

#include "CoreMinimal.h"
#include "FTFlowStateType.generated.h"

UENUM(BlueprintType)
enum class EFTFlowStateType : uint8
{
	MainMenu,
	Base,
	RaidEntering,
	RaidInProgress,
	Escaping,
	Escaped,
	Failed
};
