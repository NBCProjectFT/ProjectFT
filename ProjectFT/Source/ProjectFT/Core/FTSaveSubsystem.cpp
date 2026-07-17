#include "FTSaveSubsystem.h"

#include "EngineUtils.h"
#include "FTLogChannels.h"
#include "FTSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/Hub/FTHubStorage.h"

void UFTSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreateSave();
}

UFTSaveGame* UFTSaveSubsystem::LoadOrCreateSave()
{
	if (CurrentSave)
	{
		return CurrentSave;
	}

	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		CurrentSave = Cast<UFTSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		bHasSaveData = CurrentSave != nullptr;
	}

	if (!CurrentSave)
	{
		CurrentSave = Cast<UFTSaveGame>(UGameplayStatics::CreateSaveGameObject(UFTSaveGame::StaticClass()));
		bHasSaveData = false;
	}

	if (CurrentSave)
	{
		CurrentSave->SlotName = SlotName;
	}

	return CurrentSave;
}

bool UFTSaveSubsystem::SaveToDisk()
{
	UFTSaveGame* SaveGame = LoadOrCreateSave();
	if (!SaveGame)
	{
		UE_LOG(LogFTSave, Error, TEXT("Save failed because save object is null."));
		return false;
	}

	SaveGame->SavedAt = FDateTime::Now();
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
	bHasSaveData = bSaved || bHasSaveData;
	UE_LOG(LogFTSave, Log, TEXT("SaveGameToSlot %s. Slot=%s Level=%s FlowState=%d"),
		bSaved ? TEXT("succeeded") : TEXT("failed"),
		*SlotName,
		*SaveGame->LastLevelName.ToString(),
		static_cast<uint8>(SaveGame->FlowState));

	return bSaved;
}

void UFTSaveSubsystem::CaptureCurrentWorldState(FName NextLevelName, EFTFlowStateType FlowState)
{
	UFTSaveGame* SaveGame = LoadOrCreateSave();
	if (!SaveGame)
	{
		return;
	}

	SaveGame->LastLevelName = NextLevelName;
	SaveGame->FlowState = FlowState;

	if (UFTInventoryComponent* PlayerInventory = FindPlayerInventory())
	{
		PlayerInventory->ExportSaveState(SaveGame->PlayerInventory);
	}

	if (UFTInventoryComponent* StorageInventory = FindStorageInventory())
	{
		StorageInventory->ExportSaveState(SaveGame->StorageInventory);
	}

	if (UFTObjectiveSubsystem* ObjectiveSubsystem = FindObjectiveSubsystem())
	{
		ObjectiveSubsystem->BuildQuestSaveData(SaveGame->QuestData);
	}
}

bool UFTSaveSubsystem::SaveBeforeLevelTransition(FName NextLevelName, EFTFlowStateType FlowState)
{
	CaptureCurrentWorldState(NextLevelName, FlowState);
	return SaveToDisk();
}

int32 UFTSaveSubsystem::GetStorageSnapshotItemCount(const FName ItemID) const
{
	if (!CurrentSave || ItemID.IsNone())
	{
		return 0;
	}

	int32 ItemCount = 0;
	for (const FFTSavedInventoryItemStruct& Item : CurrentSave->StorageInventory.Items)
	{
		if (Item.ItemId == ItemID)
		{
			ItemCount += FMath::Max(0, Item.Quantity);
		}
	}

	return ItemCount;
}

void UFTSaveSubsystem::RestoreCurrentWorldState()
{
	LoadOrCreateSave();

	if (!bHasSaveData)
	{
		UE_LOG(LogFTSave, Log, TEXT("Restore skipped because no saved data exists yet."));
		return;
	}

	RestorePlayerInventory();
	RestoreStorageInventory();
	RestoreQuestState();
}

void UFTSaveSubsystem::RestoreQuestState()
{
	LoadOrCreateSave();

	if (!bHasSaveData || !CurrentSave)
	{
		return;
	}

	if (UFTObjectiveSubsystem* ObjectiveSubsystem = FindObjectiveSubsystem())
	{
		if (ObjectiveSubsystem->IsQuestDataConfigured())
		{
			ObjectiveSubsystem->RestoreQuestSaveData(CurrentSave->QuestData);
		}
	}
}

void UFTSaveSubsystem::ClearPlayerInventoryForRaidFailure()
{
	UFTSaveGame* SaveGame = LoadOrCreateSave();
	if (!SaveGame)
	{
		return;
	}

	if (UFTInventoryComponent* PlayerInventory = FindPlayerInventory())
	{
		PlayerInventory->ClearInventory();
		PlayerInventory->ExportSaveState(SaveGame->PlayerInventory);
	}
	else
	{
		SaveGame->PlayerInventory.Items.Reset();
		SaveGame->PlayerInventory.QuickSlots.Init(NAME_None, 6);
	}

	UE_LOG(LogFTSave, Log, TEXT("Player inventory cleared because raid failed."));
}

UFTInventoryComponent* UFTSaveSubsystem::FindPlayerInventory() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	return PlayerPawn ? PlayerPawn->FindComponentByClass<UFTInventoryComponent>() : nullptr;
}

UFTInventoryComponent* UFTSaveSubsystem::FindStorageInventory() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AFTHubStorage> It(World); It; ++It)
	{
		if (UFTInventoryComponent* StorageInventory = It->GetStorageInventory())
		{
			return StorageInventory;
		}
	}

	return nullptr;
}

UFTObjectiveSubsystem* UFTSaveSubsystem::FindObjectiveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFTObjectiveSubsystem>() : nullptr;
}

void UFTSaveSubsystem::RestorePlayerInventory()
{
	if (CurrentSave)
	{
		if (UFTInventoryComponent* PlayerInventory = FindPlayerInventory())
		{
			PlayerInventory->ImportSaveState(CurrentSave->PlayerInventory);
		}
	}
}

void UFTSaveSubsystem::RestoreStorageInventory()
{
	if (CurrentSave)
	{
		if (UFTInventoryComponent* StorageInventory = FindStorageInventory())
		{
			StorageInventory->ImportSaveState(CurrentSave->StorageInventory);
		}
	}
}
