#pragma once

#include "CoreMinimal.h"
#include "FTFlowStateType.generated.h"

UENUM(BlueprintType)
enum class EFTFlowStateType : uint8
{
	Base,
	RaidReady,
	RaidInProgress,
	Escaping,
	Settlement,
	Failed
};
