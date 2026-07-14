#include "FTSaveSubsystem.h"

#include "EngineUtils.h"
#include "FTLogChannels.h"
#include "FTSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
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
}

bool UFTSaveSubsystem::SaveBeforeLevelTransition(FName NextLevelName, EFTFlowStateType FlowState)
{
	CaptureCurrentWorldState(NextLevelName, FlowState);
	return SaveToDisk();
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
