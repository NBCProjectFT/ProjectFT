#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "ProjectFT/Struct/FTQuestSaveData.h"
#include "ProjectFT/Struct/FTSavedInventoryStateStruct.h"
#include "FTSaveGame.generated.h"

UCLASS()
class PROJECTFT_API UFTSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	int32 SaveVersion = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	FString SlotName = TEXT("FT_AutoSave");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	FDateTime SavedAt;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	FName LastLevelName = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	EFTFlowStateType FlowState = EFTFlowStateType::MainMenu;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	FFTSavedInventoryStateStruct PlayerInventory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	FFTSavedInventoryStateStruct StorageInventory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	FFTQuestSaveData QuestData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	TArray<FName> UnlockedRecipes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	TMap<FName, int32> BaseUpgradeState;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	TArray<FName> StoredItems;

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	void SaveProgress();
};
