#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "FTCheatManager.generated.h"

UCLASS()
class PROJECTFT_API UFTCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(exec)
	void GiveItem(const FString& ItemId, int32 Quantity = 1);

	UFUNCTION(exec)
	void SpawnItem(const FString& ItemId, int32 Quantity = 1);

	UFUNCTION(exec)
	void ShowItemList(const FString& Filter = TEXT(""));

	UFUNCTION(exec)
	void FTDumpLoadedAssets(int32 MaxCount = 50);
};
