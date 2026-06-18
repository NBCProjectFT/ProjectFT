#pragma once

#include "CoreMinimal.h"
#include "FTItemCategoryType.generated.h"

UENUM(BlueprintType)
enum class EFTItemCategoryType : uint8
{
	None            UMETA(DisplayName = "None"),
	Basic            UMETA(DisplayName = "Basic"),
	Material        UMETA(DisplayName = "Material"),
    Weapon            UMETA(DisplayName = "Weapon"),
	Armor            UMETA(DisplayName = "Armor"),
	Consumable        UMETA(DisplayName = "Consumable"),
    RecipeBook        UMETA(DisplayName = "RecipeBook"),
	Quest            UMETA(DisplayName = "Quest")
};
