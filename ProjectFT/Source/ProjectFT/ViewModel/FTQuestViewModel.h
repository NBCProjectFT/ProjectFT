#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTQuestViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECTFT_API UFTQuestViewModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "FT|Quest")
	TArray<FName> QuestList;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Quest")
	FName SelectedQuest = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Quest")
	float ObjectiveProgress = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void NotifyChanged();
};
