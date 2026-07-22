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

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Items")
	TArray<UObject*> GetQuestObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Items")
	TArray<UObject*> GetRequiredItemObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Items")
	TArray<UObject*> GetRewardItemObjects() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Selection")
	UFTQuestListObject* GetSelectedQuestObject() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|State")
	int32 GetActiveQuestCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|State")
	int32 GetCompletedQuestCount() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|State")
	EFTQuestStateType GetQuestFilter() const { return CurrentQuestFilter; }

	UFUNCTION(BlueprintPure, Category = "FT|Quest|State")
	EFTQuestStateType GetSelectedQuestState() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|State")
	bool IsActiveQuestTabSelected() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|State")
	bool IsCompletedQuestTabSelected() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Selection")
	bool HasSelectedQuestRequiredItems() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Rules")
	bool CanAcceptSelectedQuest() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Rules")
	bool CanCompleteSelectedQuest() const;

	UFUNCTION(BlueprintPure, Category = "FT|Quest|Rules")
	bool CanExecuteSelectedQuestAction() const;

	UPROPERTY(BlueprintReadWrite, Category = "FT|Quest")
	float ObjectiveProgress = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "FT|Quest")
	void RefreshAll();

	UFUNCTION(BlueprintCallable, Category = "FT|Quest|Filter")
	void SetQuestFilter(EFTQuestStateType NewQuestFilter);

	UFUNCTION(BlueprintCallable, Category = "FT|Quest|Selection")
	void SelectQuestObject(UObject* ItemObject);

	UFUNCTION(BlueprintCallable, Category = "FT|Quest|Actions")
	bool AcceptSelectedQuest();

	UFUNCTION(BlueprintCallable, Category = "FT|Quest|Actions")
	bool CompleteSelectedQuest();

	UFUNCTION(BlueprintCallable, Category = "FT|Quest|Actions")
	bool ExecuteSelectedQuestAction();

	UPROPERTY(BlueprintAssignable, Category = "FT|Quest")
	FFTQuestViewModelChanged OnChanged;

private:
	UFUNCTION()
	void HandleInventoryChanged();
	void HandleQuestStateChanged(FName QuestID);

	void RefreshQuestList();
	void RefreshSelectedQuestItems();
	void RestoreSelection(FName PreviousQuestID);
	void ClearSelection();
	void BindInventoryDelegate();
	void UnbindInventoryDelegate();
	void BindObjectiveDelegate();
	void UnbindObjectiveDelegate();
	const struct FTQuestStruct* GetSelectedQuest() const;
	void NotifyChanged();

	UPROPERTY(Transient)
	TObjectPtr<UFTObjectiveSubsystem> ObjectiveSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> PlayerInventory;

	UPROPERTY(Transient)
	TObjectPtr<UFTInventoryComponent> BoundStorageInventory;

	FDelegateHandle ObjectiveChangedDelegateHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> QuestObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RequiredItemObjects;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RewardItemObjects;

	UPROPERTY(Transient)
	TObjectPtr<UFTQuestListObject> SelectedQuestObject;

	EFTQuestStateType CurrentQuestFilter = EFTQuestStateType::Active;
	bool bTransactionInProgress = false;
};
