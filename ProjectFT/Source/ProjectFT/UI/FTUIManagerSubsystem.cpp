#include "FTUIManagerSubsystem.h"

#include "../ViewModel/FTCraftingViewModel.h"
#include "../ViewModel/FTHUDViewModel.h"
#include "../ViewModel/FTInventoryViewModel.h"
#include "../ViewModel/FTQuestViewModel.h"
#include "../ViewModel/FTSettlementViewModel.h"

void UFTUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	HUDViewModel = NewObject<UFTHUDViewModel>(this);
	InventoryViewModel = NewObject<UFTInventoryViewModel>(this);
	CraftingViewModel = NewObject<UFTCraftingViewModel>(this);
	QuestViewModel = NewObject<UFTQuestViewModel>(this);
	SettlementViewModel = NewObject<UFTSettlementViewModel>(this);
}

void UFTUIManagerSubsystem::ShowHUD()
{
}

void UFTUIManagerSubsystem::ShowInventory()
{
}

void UFTUIManagerSubsystem::ShowCrafting()
{
}

void UFTUIManagerSubsystem::ShowStorage()
{
}

void UFTUIManagerSubsystem::ShowQuestBoard()
{
}

void UFTUIManagerSubsystem::ShowFailScreen()
{
}

void UFTUIManagerSubsystem::ShowSettlementScreen()
{
}
