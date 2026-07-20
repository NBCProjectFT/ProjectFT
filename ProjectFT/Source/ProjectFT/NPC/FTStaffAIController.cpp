#include "FTStaffAIController.h"

#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

AFTStaffAIController::AFTStaffAIController()
{
}

void AFTStaffAIController::BeginPlay()
{
	Super::BeginPlay();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	StealCompletedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_StealCompleted,
		this,
		&ThisClass::OnStealCompleted
	);
	ShelfRestockedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_ShelfRestocked,
		this,
		&ThisClass::OnShelfRestocked
	);
}

void AFTStaffAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StealCompletedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(StealCompletedListenerHandle);
		StealCompletedListenerHandle = FGameplayMessageListenerHandle();
	}
	if (ShelfRestockedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(ShelfRestockedListenerHandle);
		ShelfRestockedListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::EndPlay(EndPlayReason);
}

bool AFTStaffAIController::BroadcastRestockRequested()
{
	if (!TargetShelfActor || bRestockRequested)
	{
		return false;
	}

	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetPawn();
	Payload.TargetActor = TargetShelfActor;
	Payload.Value = 1.0f;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_ShelfRestockRequested, Payload);
	bRestockRequested = true;

	if (bLogStaffDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[Staff] Restock requested: Staff=%s Shelf=%s"),
			*GetNameSafe(GetPawn()),
			*GetNameSafe(TargetShelfActor)
		);
	}

	return true;
}

void AFTStaffAIController::ClearRestockTarget()
{
	TargetShelfActor = nullptr;
	RestockLocation = FVector::ZeroVector;
	bHasRestockTarget = false;
	bRestockRequested = false;
	bRestockCompleted = false;
}

void AFTStaffAIController::OnStealCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	if (!Payload.TargetActor || bHasRestockTarget)
	{
		return;
	}

	TargetShelfActor = Payload.TargetActor;
	RestockLocation = Payload.TargetActor->GetActorLocation();
	bHasRestockTarget = true;
	bRestockRequested = false;
	bRestockCompleted = false;

	if (bLogStaffDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[Staff] Restock target assigned: Staff=%s Shelf=%s Location=%s"),
			*GetNameSafe(GetPawn()),
			*GetNameSafe(TargetShelfActor),
			*RestockLocation.ToString()
		);
	}
}

void AFTStaffAIController::OnShelfRestocked(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	if (!TargetShelfActor || Payload.TargetActor != TargetShelfActor)
	{
		return;
	}

	bRestockCompleted = true;

	if (bLogStaffDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[Staff] Restock completed: Staff=%s Shelf=%s"),
			*GetNameSafe(GetPawn()),
			*GetNameSafe(TargetShelfActor)
		);
	}
}
