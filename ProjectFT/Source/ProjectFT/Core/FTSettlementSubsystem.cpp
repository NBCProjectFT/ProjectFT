#include "FTSettlementSubsystem.h"

#include "FTSaveGame.h"

void UFTSettlementSubsystem::CalculateSettlement()
{
}

void UFTSettlementSubsystem::ApplyResult(UFTSaveGame* SaveGame)
{
	if (!SaveGame)
	{
		return;
	}

	for (const FName& RecipeId : UnlockedRecipes)
	{
		SaveGame->UnlockedRecipes.AddUnique(RecipeId);
	}
}
