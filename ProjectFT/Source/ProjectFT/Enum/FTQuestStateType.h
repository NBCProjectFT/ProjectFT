#pragma once

#include "CoreMinimal.h"
#include "FTQuestStateType.generated.h"

UENUM(BlueprintType)
enum class EFTQuestStateType : uint8
{
	Locked,
	Available,
	Completed
};