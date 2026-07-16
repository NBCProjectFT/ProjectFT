#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Enum/FTQuestStateType.h"
#include "ProjectFT/Struct/FTQuestSaveData.h"
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
	void NotifyEscapeReached();

	UFUNCTION(BlueprintPure, Category = "FT|Objective")
	float GetQuestProgress(FName QuestID) const;

	UFUNCTION(BlueprintPure, Category = "FT|Objective")
	FText GetQuestProgressText(FName QuestID) const;

	/**
	 * HUD 상세 목표용 멀티라인 텍스트를 만든다.
	 * ObjectiveLines는 RequiredItems 순서, 그 다음 EventConditions 순서에 대응한다.
	 */
	UFUNCTION(BlueprintPure, Category = "FT|Objective")
	FText GetQuestObjectiveProgressText(FName QuestID) const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest")
	bool IsQuestDataConfigured() const { return QuestDataTable != nullptr; }

	void ConfigureHubQuests(
		UDataTable* InQuestDataTable,
		AFTHubStorage* InHubStorage,
		const TArray<FName>& InInitialQuestIDs
	);

	bool CanCompleteQuest(const FTQuestStruct& Quest, UFTInventoryComponent* PlayerInventory) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	bool TryCompleteQuest(FName QuestID, UFTInventoryComponent* PlayerInventory);

	/** 현재 퀘스트 상태와 이벤트 진행도를 세이브 가능한 스냅샷으로 복사한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Quest|Save")
	void BuildQuestSaveData(FFTQuestSaveData& OutSaveData) const;

	/**
	 * 저장된 퀘스트 스냅샷을 복원한다.
	 * QuestDataTable이 연결된 뒤 호출해야 하며, 잘못된 ID와 변경된 조건 배열은 현재 데이터에 맞춰 보정한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Quest|Save")
	void RestoreQuestSaveData(const FFTQuestSaveData& SaveData);

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
	void HandleQuestMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void HandleNPCQuestMessage(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void HandleSecurityChaseQuestMessage(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void HandleSecurityResponseQuestMessage(FGameplayTag Channel, const FFTSecurityResponsePayloadStruct& Payload);
	void HandleRaidEscapedMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void ApplyQuestEvent(FGameplayTag EventTag, FName ItemID = NAME_None, int32 Count = 1);
	void ActivateQuestProgress(const FTQuestStruct& Quest);
	void BroadcastQuestProgressChanged(FName QuestID) const;
	const FTQuestStruct* FindQuestByID(FName QuestID) const;
	int32 GetQuestRequiredTotal(const FTQuestStruct& Quest) const;
	int32 GetQuestItemProgressTotal(const FTQuestStruct& Quest, UFTInventoryComponent* PlayerInventory) const;
	int32 GetQuestEventRequiredTotal(const FTQuestStruct& Quest) const;
	int32 GetQuestEventProgressTotal(const FTQuestStruct& Quest) const;
	bool AreQuestEventConditionsCompleted(const FTQuestStruct& Quest) const;
	bool CanGrantQuestRewards(const FTQuestStruct& Quest, UFTInventoryComponent* PlayerInventory) const;
	int32 GetCombinedItemCount(UFTInventoryComponent* PlayerInventory, FName ItemID) const;
	bool ConsumeCombinedItem(UFTInventoryComponent* PlayerInventory, FName ItemID, int32 Count);
	UFTInventoryComponent* ResolvePlayerInventory() const;

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
