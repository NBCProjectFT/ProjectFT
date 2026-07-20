#include "FTStaffAIController.h"

#include "EngineUtils.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/NPC/FTShoppingPoint.h"
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
	ClearStaffWanderTarget();

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

bool AFTStaffAIController::PickRandomStaffWanderTarget()
{
	if (bHasRestockTarget)
	{
		return false;
	}

	ClearStaffWanderTarget();

	TArray<AFTShoppingPoint*> PreferredShoppingPoints;
	TArray<AFTShoppingPoint*> FallbackShoppingPoints;
	float PreferredTotalWeight = 0.0f;
	float FallbackTotalWeight = 0.0f;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AFTShoppingPoint> It(World); It; ++It)
		{
			AFTShoppingPoint* ShoppingPoint = *It;
			if (!ShoppingPoint || ShoppingPoint->SelectionWeight <= 0.0f)
			{
				continue;
			}

			FallbackShoppingPoints.Add(ShoppingPoint);
			FallbackTotalWeight += ShoppingPoint->SelectionWeight;

			if (ShoppingPoint->CanSelectPreferred())
			{
				PreferredShoppingPoints.Add(ShoppingPoint);
				PreferredTotalWeight += ShoppingPoint->SelectionWeight;
			}
		}
	}

	const bool bUsePreferredPoints = !PreferredShoppingPoints.IsEmpty();
	const TArray<AFTShoppingPoint*>& ShoppingPoints = bUsePreferredPoints ? PreferredShoppingPoints : FallbackShoppingPoints;
	const float TotalWeight = bUsePreferredPoints ? PreferredTotalWeight : FallbackTotalWeight;

	if (ShoppingPoints.IsEmpty() || TotalWeight <= 0.0f)
	{
		StaffWanderLocation = FVector::ZeroVector;
		StaffWanderAcceptanceRadius = 100.0f;
		bHasStaffWanderTarget = false;
		return false;
	}

	// 여유가 있는 쇼핑 포인트를 우선 선택하고, 없으면 전체 포인트 중 가중치로 선택합니다.
	AFTShoppingPoint* SelectedShoppingPoint = nullptr;
	float RandomWeight = FMath::FRandRange(0.0f, TotalWeight);
	for (AFTShoppingPoint* ShoppingPoint : ShoppingPoints)
	{
		RandomWeight -= ShoppingPoint->SelectionWeight;
		if (RandomWeight <= 0.0f)
		{
			SelectedShoppingPoint = ShoppingPoint;
			break;
		}
	}

	if (!SelectedShoppingPoint)
	{
		SelectedShoppingPoint = ShoppingPoints.Last();
	}

	CurrentStaffWanderPoint = SelectedShoppingPoint;
	SelectedShoppingPoint->Reserve();
	SelectedShoppingPoint->GetRandomShoppingLocation(this, StaffWanderLocation);
	StaffWanderAcceptanceRadius = SelectedShoppingPoint->AcceptanceRadius;
	bHasStaffWanderTarget = true;

	if (bLogStaffDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[Staff] Wander target picked: Staff=%s Point=%s Location=%s"),
			*GetNameSafe(GetPawn()),
			*GetNameSafe(SelectedShoppingPoint),
			*StaffWanderLocation.ToString()
		);
	}

	return true;
}

void AFTStaffAIController::ClearStaffWanderTarget()
{
	if (CurrentStaffWanderPoint)
	{
		CurrentStaffWanderPoint->Release();
		CurrentStaffWanderPoint = nullptr;
	}

	StaffWanderLocation = FVector::ZeroVector;
	StaffWanderAcceptanceRadius = 100.0f;
	bHasStaffWanderTarget = false;
}

void AFTStaffAIController::OnStealCompleted(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	if (!Payload.TargetActor || bHasRestockTarget)
	{
		return;
	}

	ClearStaffWanderTarget();

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
