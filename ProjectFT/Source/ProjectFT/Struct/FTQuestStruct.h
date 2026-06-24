#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "FTQuestStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTFT_API FTQuestStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText QuestName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTCraftIngredientStruct> RequiredItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTCraftIngredientStruct> RewardItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> NextQuestIDs;
};