#include "FTNPCShoppingComponent.h"

#include "AIController.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/NPC/FTNPCAIController.h"
#include "ProjectFT/NPC/FTShoppingPoint.h"

UFTNPCShoppingComponent::UFTNPCShoppingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFTNPCShoppingComponent::PickRandomShoppingTarget()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return false;
	}

	ReleaseShoppingTarget();

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

	const TArray<AFTShoppingPoint*>& ShoppingPoints = PreferredShoppingPoints.IsEmpty()
		? FallbackShoppingPoints
		: PreferredShoppingPoints;
	const float TotalWeight = PreferredShoppingPoints.IsEmpty()
		? FallbackTotalWeight
		: PreferredTotalWeight;

	if (ShoppingPoints.IsEmpty() || TotalWeight <= 0.0f)
	{
		Controller->ShoppingTargetLocation = FVector::ZeroVector;
		Controller->ShoppingLookLocation = FVector::ZeroVector;
		Controller->ShoppingTargetAcceptanceRadius = 100.0f;
		Controller->bHasShoppingTarget = false;
		if (Controller->bLogShoppingDebug)
		{
			UE_LOG(LogFTNPC, Warning, TEXT("NPC AI: No available shopping point found"));
		}
		return false;
	}

	// 여유 슬롯이 있는 포인트를 우선 선택하고, 없으면 모든 포인트 중에서 가중치로 선택한다.
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

	CurrentShoppingPoint = SelectedShoppingPoint;
	SelectedShoppingPoint->Reserve();
	SelectedShoppingPoint->GetRandomShoppingLocation(this, Controller->ShoppingTargetLocation);
	Controller->ShoppingLookLocation = Controller->ShoppingTargetLocation + SelectedShoppingPoint->GetActorForwardVector() * 500.0f;
	Controller->ShoppingTargetAcceptanceRadius = SelectedShoppingPoint->AcceptanceRadius;
	Controller->bHasShoppingTarget = true;

	if (Controller->bLogShoppingDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("NPC AI: Picked shopping target %s at %s"),
			*SelectedShoppingPoint->GetName(),
			*Controller->ShoppingTargetLocation.ToString());
	}
	return true;
}

void UFTNPCShoppingComponent::ReleaseShoppingTarget()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	if (CurrentShoppingPoint)
	{
		CurrentShoppingPoint->Release();
		CurrentShoppingPoint = nullptr;
	}

	// 다음 쇼핑 목적지로 이동할 때 이전 시선 보간 상태를 정리한다.
	ClearShoppingFocusState();
	Controller->bHasShoppingTarget = false;
	if (!Controller->IsUsingReportFocus())
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void UFTNPCShoppingComponent::StartShoppingLook()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller || !Controller->bHasShoppingTarget)
	{
		return;
	}

	// 현재 바라보는 방향에서 쇼핑 목표 방향으로 천천히 보간하기 위해 목표 지점만 저장한다.
	DesiredShoppingLookLocation = Controller->ShoppingLookLocation;
	if (CurrentShoppingLookLocation.IsNearlyZero())
	{
		if (const APawn* ControlledPawn = Controller->GetPawn())
		{
			CurrentShoppingLookLocation = ControlledPawn->GetActorLocation() + ControlledPawn->GetActorForwardVector() * 500.0f;
		}
		else
		{
			CurrentShoppingLookLocation = DesiredShoppingLookLocation;
		}
	}

	bBlendShoppingLook = true;
}

void UFTNPCShoppingComponent::TickShoppingLook(float DeltaTime)
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller || Controller->bFleeRequested || Controller->bPanicRequested || Controller->bKnockedOut || !bBlendShoppingLook || Controller->IsUsingReportFocus())
	{
		return;
	}

	// Focus 지점을 바로 바꾸지 않고 보간해서 손님 NPC의 시선 전환이 갑자기 꺾이지 않게 한다.
	CurrentShoppingLookLocation = FMath::VInterpTo(
		CurrentShoppingLookLocation,
		DesiredShoppingLookLocation,
		DeltaTime,
		Controller->ShoppingLookInterpSpeed);

	Controller->SetFocalPoint(CurrentShoppingLookLocation, EAIFocusPriority::Gameplay);

	if (FVector::DistSquared(CurrentShoppingLookLocation, DesiredShoppingLookLocation) > FMath::Square(10.0f))
	{
		return;
	}

	CurrentShoppingLookLocation = DesiredShoppingLookLocation;
	Controller->SetFocalPoint(CurrentShoppingLookLocation, EAIFocusPriority::Gameplay);
	bBlendShoppingLook = false;
}

void UFTNPCShoppingComponent::ClearShoppingFocusState()
{
	bBlendShoppingLook = false;
	CurrentShoppingLookLocation = FVector::ZeroVector;
	DesiredShoppingLookLocation = FVector::ZeroVector;
}

AFTNPCAIController* UFTNPCShoppingComponent::GetNPCAIController() const
{
	return Cast<AFTNPCAIController>(GetOwner());
}
