#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTQuestBoardWidget.generated.h"

UCLASS()
class PROJECTFT_API UFTQuestBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void RefreshQuestList();

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void ShowQuestDetail(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void RequestSelectQuest(FName QuestId);
};
