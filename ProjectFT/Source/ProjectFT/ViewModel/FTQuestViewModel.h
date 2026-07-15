#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Enum/FTQuestStateType.h"
#include "FTQuestViewModel.generated.h"

class UFTObjectiveSubsystem;
class UFTInventoryComponent;
class UFTQuestListObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTQuestViewModelChanged);

UCLASS(BlueprintType)
class PROJECTFT_API UFTQuestViewModel : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFTObjectiveSubsystem* InObjectiveSubsystem, UFTInventoryComponent* InPlayerInventory);

	const TArray<TObjectPtr<UObject>>& GetQuestObjects() const;
	const TArray<TObjectPtr<UObject>>& GetRequiredItemObjects() const;
	const TArray<TObjectPtr<UObject>>& GetRewardItemObjects() const;
	UFTQuestListObject* GetSelectedQuestObject() const;

	FText GetSelectedQuestNameText() const;
	FText GetSelectedQuestSenderText() const;
	FText GetSelectedQuestDescriptionText() const;
	FText GetSelectedQuestObjectiveLinesText() const;
	FText GetSelectedQuestCurrencyRewardText() const;
	FText GetSelectedQuestActionText() const;
	FText GetActiveQuestCountText() const;
	FText GetCompletedQuestCountText() const;
	bool IsActiveQuestTabSelected() const;
	bool IsCompletedQuestTabSelected() const;
	bool HasSelectedQuestRequiredItems() const;
	bool CanAcceptSelectedQuest() const;
	bool CanCompleteSelectedQuest() const;
	bool CanExecuteSelectedQuestAction() const;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Quest")
	float ObjectiveProgress = 0.0f;

	void RefreshAll();
	void SetQuestFilter(EFTQuestStateType NewQuestFilter);
	void SelectQuestObject(UObject* ItemObject);
	bool AcceptSelectedQuest();
	bool CompleteSelectedQuest();
	bool ExecuteSelectedQuestAction();

	UPROPERTY(BlueprintAssignable, Category = "FT|Quest")
	FFTQuestViewModelChanged OnChanged;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void RefreshQuestList();
	void RefreshSelectedQuestItems();
	void RestoreSelection(FName PreviousQuestID);
	void ClearSelection();
	void BindInventoryDelegate();
	void UnbindInventoryDelegate();
	const struct FTQuestStruct* GetSelectedQuest() const;
	void NotifyChanged();

	UPROPERTY(Transient)
	TObjectPtr<UFTObjectiveSubsystem> ObjectiveSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> BoundStorageInventory;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> QuestObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RequiredItemObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RewardItemObjects;

	UPROPERTY(Transient)
	TObjectPtr<UFTQuestListObject> SelectedQuestObject;

	EFTQuestStateType CurrentQuestFilter = EFTQuestStateType::Active;
};
