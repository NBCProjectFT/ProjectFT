#include "FTStaffRestockManager.h"

#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

AFTStaffRestockManager::AFTStaffRestockManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFTStaffRestockManager::BeginPlay()
{
	Super::BeginPlay();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	StealCompletedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_StealCompleted,
		this,
		&ThisClass::OnStealCompleted
	);
}

void AFTStaffRestockManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StealCompletedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(StealCompletedListenerHandle);
		StealCompletedListenerHandle = FGameplayMessageListenerHandle();
	}

	PendingShelves.Empty();
	AssignedShelves.Empty();

	Super::EndPlay(EndPlayReason);
}

void AFTStaffRestockManager::RegisterEmptyShelf(AActor* ShelfActor)
{
	if (!ShelfActor)
	{
		return;
	}

	RemoveInvalidShelves();

	const bool bAlreadyPending = PendingShelves.ContainsByPredicate([ShelfActor](const TWeakObjectPtr<AActor>& ShelfPtr)
	{
		return ShelfPtr.Get() == ShelfActor;
	});

	if (bAlreadyPending)
	{
		return;
	}

	PendingShelves.Add(ShelfActor);

	if (bLogRestockDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[StaffRestock] Empty shelf registered: Shelf=%s"), *GetNameSafe(ShelfActor));
	}
}

bool AFTStaffRestockManager::TryAssignShelf(AActor* StaffActor, AActor*& OutShelfActor)
{
	OutShelfActor = nullptr;
	if (!StaffActor)
	{
		return false;
	}

	RemoveInvalidShelves();

	for (const TWeakObjectPtr<AActor>& ShelfPtr : PendingShelves)
	{
		AActor* ShelfActor = ShelfPtr.Get();
		if (!ShelfActor || AssignedShelves.Contains(ShelfPtr))
		{
			continue;
		}

		AssignedShelves.Add(ShelfPtr, StaffActor);
		OutShelfActor = ShelfActor;

		if (bLogRestockDebug)
		{
			UE_LOG(
				LogFTNPC,
				Log,
				TEXT("[StaffRestock] Shelf assigned: Staff=%s Shelf=%s"),
				*GetNameSafe(StaffActor),
				*GetNameSafe(ShelfActor)
			);
		}

		return true;
	}

	return false;
}

void AFTStaffRestockManager::CompleteShelf(AActor* ShelfActor)
{
	if (!ShelfActor)
	{
		return;
	}

	PendingShelves.RemoveAll([ShelfActor](const TWeakObjectPtr<AActor>& ShelfPtr)
	{
		return ShelfPtr.Get() == ShelfActor;
	});

	for (auto It = AssignedShelves.CreateIterator(); It; ++It)
	{
		if (It.Key().Get() == ShelfActor)
		{
			It.RemoveCurrent();
			break;
		}
	}

	if (bLogRestockDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[StaffRestock] Shelf completed: Shelf=%s"), *GetNameSafe(ShelfActor));
	}
}

void AFTStaffRestockManager::OnStealCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	RegisterEmptyShelf(Payload.TargetActor);
}

void AFTStaffRestockManager::RemoveInvalidShelves()
{
	PendingShelves.RemoveAll([](const TWeakObjectPtr<AActor>& ShelfPtr)
	{
		return !ShelfPtr.IsValid();
	});

	for (auto It = AssignedShelves.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !It.Value().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}
