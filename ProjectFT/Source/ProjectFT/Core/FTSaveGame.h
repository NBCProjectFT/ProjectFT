#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FTSaveGame.generated.h"

UCLASS()
class PROJECTFT_API UFTSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	TArray<FName> UnlockedRecipes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	TMap<FName, int32> BaseUpgradeState;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FT|Save")
	TArray<FName> StoredItems;

	UFUNCTION(BlueprintCallable, Category = "FT|Save")
	void SaveProgress();
};
