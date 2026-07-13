#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Enum/FTQuestStateType.h"
#include "ProjectFT/Struct/FTQuestStruct.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTObjectiveSubsystem.generated.h"

class AFTHubStorage;
class UDataTable;
class UFTInventoryComponent;
struct FFTMessagePayloadStruct;
struct FFTNPCReportPayloadStruct;
struct FFTSecurityChaseGaugePayloadStruct;
struct FFTSecurityResponsePayloadStruct;

UCLASS()
class PROJECTFT_API UFTObjectiveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

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

	UFUNCTION(BlueprintPure, Category = "FT|Objective")
	float GetQuestProgress(FName QuestID) const;

	UFUNCTION(BlueprintPure, Category = "FT|Objective")
	FText GetQuestProgressText(FName QuestID) const;

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
	void HandleItemPickedUpMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void HandleQuestMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void HandleNPCQuestMessage(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void HandleSecurityChaseQuestMessage(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void HandleSecurityResponseQuestMessage(FGameplayTag Channel, const FFTSecurityResponsePayloadStruct& Payload);
	void HandleRaidEscapedMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void ApplyQuestEvent(FGameplayTag EventTag, FName ItemID = NAME_None, int32 Count = 1);
	void ActivateQuestProgress(const FTQuestStruct& Quest);
	void BroadcastQuestProgressChanged(FName QuestID) const;
	bool IsItemRequiredByActiveQuest(FName ItemID) const;
	const FTQuestStruct* FindQuestByID(FName QuestID) const;
	int32 GetRequiredItemCountForQuest(const FTQuestStruct& Quest, FName ItemID) const;
	int32 GetPickedUpItemCount(FName ItemID) const;
	int32 GetQuestRequiredTotal(const FTQuestStruct& Quest) const;
	int32 GetQuestPickedUpTotal(const FTQuestStruct& Quest) const;
	int32 GetQuestEventRequiredTotal(const FTQuestStruct& Quest) const;
	int32 GetQuestEventProgressTotal(const FTQuestStruct& Quest) const;
	bool AreQuestEventConditionsCompleted(const FTQuestStruct& Quest) const;
	int32 GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const;
	bool ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count);

	UPROPERTY()
	TSet<FName> PickedUpRequiredItems;

	UPROPERTY(Transient)
	TMap<FName, int32> PickedUpItemCounts;

	/** QuestID별 EventConditions 진행 횟수. 배열 인덱스는 조건 배열 인덱스와 같다. */
	TMap<FName, TArray<int32>> EventConditionProgressByQuest;

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

	TArray<FGameplayMessageListenerHandle> ObjectiveListenerHandles;
};
