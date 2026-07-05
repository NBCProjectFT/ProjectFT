#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Enum/FTQuestStateType.h"
#include "ProjectFT/Struct/FTQuestStruct.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTObjectiveSubsystem.generated.h"

class AFTHubStorage;
class UDataTable;
class UFTInventoryComponent;

UCLASS()
class PROJECTFT_API UFTObjectiveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "FT|Objective")
	FName CurrentQuestId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Objective")
	TSet<FName> RequiredItems;

	UFUNCTION(BlueprintCallable, Category = "FT|Objective")
	bool IsObjectiveCompleted() const;

	UFUNCTION(BlueprintCallable, Category = "FT|Objective")
	void NotifyItemPickedUp(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "FT|Objective")
	void NotifyEscapeReached();

	void ConfigureHubQuests(
		UDataTable* InQuestDataTable,
		AFTHubStorage* InHubStorage,
		const TArray<FName>& InInitialQuestIDs
	);

	bool CanCompleteQuest(const FTQuestStruct& Quest, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	bool TryCompleteQuest(FName QuestID, UFTInventoryComponent* PlayerInventory);

	void GetQuestList(TArray<FTQuestStruct>& OutQuests) const;
	void GetQuestListByState(EFTQuestStateType QuestState, TArray<FTQuestStruct>& OutQuests) const;

	AFTHubStorage* GetHubStorage() const;

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	bool AcceptQuest(FName QuestID);

	bool IsQuestAvailable(FName QuestID) const;
	bool IsQuestActive(FName QuestID) const;
	bool IsQuestCompleted(FName QuestID) const;
	EFTQuestStateType GetQuestState(FName QuestID) const;
	void UnlockQuest(FName QuestID);

private:
	const FTQuestStruct* FindQuestByID(FName QuestID) const;
	int32 GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const;
	bool ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count);

	UPROPERTY()
	TSet<FName> PickedUpRequiredItems;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> QuestDataTable = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AFTHubStorage> HubStorage = nullptr;

	UPROPERTY(Transient)
	TSet<FName> AvailableQuestIDs;

	UPROPERTY(Transient)
	TSet<FName> ActiveQuestIDs;

	UPROPERTY(Transient)
	TSet<FName> CompletedQuestIDs;
};
