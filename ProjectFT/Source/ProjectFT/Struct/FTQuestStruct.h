#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTQuestConditionStruct.h"
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
	FText SenderName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FText> ObjectiveLines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTCraftIngredientStruct> RequiredItems;

	/** 마트 안에서 발생하는 GameplayMessage 기반 조건 목록. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FFTQuestConditionStruct> EventConditions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTCraftIngredientStruct> RewardItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 CurrencyReward = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> UnlockedShopItemIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> NextQuestIDs;
};
