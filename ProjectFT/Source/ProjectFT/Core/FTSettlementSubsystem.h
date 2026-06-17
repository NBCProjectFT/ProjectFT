#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FTSettlementSubsystem.generated.h"

class UFTSaveGame;

UCLASS()
class PROJECTFT_API UFTSettlementSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "FT|Settlement")
	TArray<FName> CollectedItems;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Settlement")
	TArray<FName> LostItems;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Settlement")
	TArray<FName> UnlockedRecipes;

	UFUNCTION(BlueprintCallable, Category = "FT|Settlement")
	void CalculateSettlement();

	UFUNCTION(BlueprintCallable, Category = "FT|Settlement")
	void ApplyResult(UFTSaveGame* SaveGame);
};
