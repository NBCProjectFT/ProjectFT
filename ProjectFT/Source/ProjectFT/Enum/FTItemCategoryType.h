#pragma once

#include "CoreMinimal.h"
#include "FTItemCategoryType.generated.h"

UENUM(BlueprintType)
enum class EFTItemCategoryType : uint8
{
	None            UMETA(DisplayName = "None"),
	Common            UMETA(DisplayName = "Common"),
	Weapon            UMETA(DisplayName = "Weapon"),
	Healing            UMETA(DisplayName = "Healing"),
	Projectile            UMETA(DisplayName = "Projectile")
};