#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Struct/FTQuestStruct.h"
#include "FTHubQuestBoard.generated.h"

class AFTHubStorage;

UCLASS()
class PROJECTFT_API AFTHubQuestBoard : public AActor
{
	GENERATED_BODY()

public:
	AFTHubQuestBoard();

	bool CanCompleteQuest(const FTQuestStruct& Quest) const;

	bool TryCompleteQuest(FName QuestID);

	void GetQuestList(TArray<FTQuestStruct>& OutQuests) const;

	AFTHubStorage* GetHubStorage() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	UDataTable* QuestDataTable;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quest")
	AFTHubStorage* HubStorage;

private:
	const FTQuestStruct* FindQuestByID(FName QuestID) const;
};