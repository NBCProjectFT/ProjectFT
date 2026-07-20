#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "FTSaveSubsystem.generated.h"

class UFTInventoryComponent;
class UFTSaveGame;

UCLASS()
class PROJECTFT_API UFTSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	UFTSaveGame* LoadOrCreateSave();

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	bool SaveToDisk();

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	void CaptureCurrentWorldState(FName NextLevelName, EFTFlowStateType FlowState);

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	bool SaveBeforeLevelTransition(FName NextLevelName, EFTFlowStateType FlowState);

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	void RestoreCurrentWorldState();

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	void RestoreQuestState();

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	void ClearPlayerInventoryForRaidFailure();

	UFUNCTION(BlueprintPure, Category = "FT|Save")
	bool HasSaveData() const { return bHasSaveData; }

	/**
	 * Returns an item count from the storage snapshot already held in memory.
	 * This is read-only and does not load, save, or mutate the current save game.
	 */
	int32 GetStorageSnapshotItemCount(FName ItemID) const;

private:
	UFTInventoryComponent* FindPlayerInventory() const;
	UFTInventoryComponent* FindStorageInventory() const;
	class UFTObjectiveSubsystem* FindObjectiveSubsystem() const;
	void RestorePlayerInventory();
	void RestoreStorageInventory();

private:
	UPROPERTY(Transient)
	TObjectPtr<UFTSaveGame> CurrentSave = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "FT|Save")
	FString SlotName = TEXT("FT_AutoSave");

	UPROPERTY(EditDefaultsOnly, Category = "FT|Save")
	int32 UserIndex = 0;

	bool bHasSaveData = false;
};
