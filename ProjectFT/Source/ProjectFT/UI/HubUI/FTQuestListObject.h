#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTQuestStruct.h"
#include "FTQuestListObject.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTQuestListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTQuestStruct& InQuest, bool bInCanComplete, bool bInAccepted);

	const FTQuestStruct& GetQuest() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	FName GetQuestID() const { return Quest.QuestID; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	FText GetQuestName() const { return Quest.QuestName; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	FText GetSenderName() const { return Quest.SenderName; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	FText GetDescription() const { return Quest.Description; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	TArray<FText> GetObjectiveLines() const { return Quest.ObjectiveLines; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	FText GetObjectiveLinesText() const { return ObjectiveLinesText; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	FText GetObjectiveSummary() const { return ObjectiveSummary; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	FText GetRewardSummary() const { return RewardSummary; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Data")
	int32 GetCurrencyReward() const { return Quest.CurrencyReward; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|State")
	bool CanComplete() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|State")
	bool IsAccepted() const;

private:
	FTQuestStruct Quest;
	FText ObjectiveLinesText;
	FText ObjectiveSummary;
	FText RewardSummary;
	bool bCanComplete = false;
	bool bAccepted = false;
};
