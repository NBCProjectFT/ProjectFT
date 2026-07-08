#include "FTSecurityCoordinationComponent.h"

#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Security/FTSecurityAIController.h"
#include "NavigationSystem.h"

UFTSecurityCoordinationComponent::UFTSecurityCoordinationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFTSecurityCoordinationComponent::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.TickInterval = SelectionInterval;
}

void UFTSecurityCoordinationComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateAttackLeader();
	UpdateEncircleSlots();
}

void UFTSecurityCoordinationComponent::RegisterSecurityController(
	AFTSecurityAIController* SecurityController)
{
	if (IsValid(SecurityController))
	{
		RegisteredSecurityControllers.Add(SecurityController);
		SecurityController->bIsAttackLeader = false;
		SecurityController->bHasEncircleSlot = false;
	}
}

void UFTSecurityCoordinationComponent::UnregisterSecurityController(
	AFTSecurityAIController* SecurityController)
{
	if (!SecurityController)
	{
		return;
	}

	RegisteredSecurityControllers.Remove(SecurityController);
	SecurityController->bIsAttackLeader = false;
	SecurityController->bHasEncircleSlot = false;

	if (AttackLeader.Get() == SecurityController)
	{
		AttackLeader.Reset();
		UpdateAttackLeader();
	}
}

AFTSecurityAIController* UFTSecurityCoordinationComponent::GetAttackLeader() const
{
	return AttackLeader.Get();
}

bool UFTSecurityCoordinationComponent::IsEligibleAttackLeader(
	const AFTSecurityAIController* SecurityController) const
{
	return IsValid(SecurityController)
		&& IsValid(SecurityController->TargetActor)
		&& SecurityController->bSecurityChaseActive
		&& SecurityController->bHasSeenTarget
		&& SecurityController->bParticipatingInChase
		&& !SecurityController->bReturning
		&& !SecurityController->bReturnRequested
		&& !SecurityController->bTargetCaptured
		&& !SecurityController->bIsGrabbing
		&& !SecurityController->bIsStunned;
}

void UFTSecurityCoordinationComponent::UpdateAttackLeader()
{
	RemoveInvalidControllers();

	AFTSecurityAIController* ClosestCandidate = nullptr;
	float ClosestDistance = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<AFTSecurityAIController>& ControllerEntry : RegisteredSecurityControllers)
	{
		AFTSecurityAIController* SecurityController = ControllerEntry.Get();
		if (!IsEligibleAttackLeader(SecurityController))
		{
			continue;
		}

		if (SecurityController->TargetDistance < ClosestDistance)
		{
			ClosestCandidate = SecurityController;
			ClosestDistance = SecurityController->TargetDistance;
		}
	}

	AFTSecurityAIController* CurrentLeader = AttackLeader.Get();
	if (IsEligibleAttackLeader(CurrentLeader))
	{
		if (!ClosestCandidate
			|| ClosestCandidate == CurrentLeader
			|| ClosestDistance + LeaderSwitchDistanceAdvantage >= CurrentLeader->TargetDistance)
		{
			SetAttackLeader(CurrentLeader);
			return;
		}
	}

	SetAttackLeader(ClosestCandidate);
}

void UFTSecurityCoordinationComponent::SetAttackLeader(
	AFTSecurityAIController* NewAttackLeader)
{
	AFTSecurityAIController* PreviousLeader = AttackLeader.Get();
	if (PreviousLeader == NewAttackLeader)
	{
		if (NewAttackLeader)
		{
			NewAttackLeader->bIsAttackLeader = true;
		}
		return;
	}

	for (const TWeakObjectPtr<AFTSecurityAIController>& ControllerEntry : RegisteredSecurityControllers)
	{
		if (AFTSecurityAIController* SecurityController = ControllerEntry.Get())
		{
			SecurityController->bIsAttackLeader = SecurityController == NewAttackLeader;
		}
	}

	AttackLeader = NewAttackLeader;
	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Security attack leader changed: %s -> %s"),
		*GetNameSafe(PreviousLeader),
		*GetNameSafe(NewAttackLeader));
}

void UFTSecurityCoordinationComponent::UpdateEncircleSlots()
{
	TMap<TObjectPtr<AActor>, TArray<AFTSecurityAIController*>> ControllersByTarget;

	for (const TWeakObjectPtr<AFTSecurityAIController>& ControllerEntry : RegisteredSecurityControllers)
	{
		AFTSecurityAIController* SecurityController = ControllerEntry.Get();
		if (!SecurityController)
		{
			continue;
		}

		SecurityController->bHasEncircleSlot = false;
		if (!IsValid(SecurityController->TargetActor)
			|| !SecurityController->bSecurityChaseActive
			|| SecurityController->bIsAttackLeader
			|| SecurityController->bReturning
			|| SecurityController->bReturnRequested
			|| SecurityController->bTargetCaptured
			|| SecurityController->bIsGrabbing
			|| SecurityController->bIsStunned)
		{
			continue;
		}

		ControllersByTarget.FindOrAdd(SecurityController->TargetActor).Add(SecurityController);
	}

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	for (TPair<TObjectPtr<AActor>, TArray<AFTSecurityAIController*>>& TargetGroup : ControllersByTarget)
	{
		AActor* TargetActor = TargetGroup.Key.Get();
		TArray<AFTSecurityAIController*>& SecurityControllers = TargetGroup.Value;
		if (!TargetActor || SecurityControllers.IsEmpty())
		{
			continue;
		}

		SecurityControllers.Sort([](const AFTSecurityAIController& Left, const AFTSecurityAIController& Right)
		{
			return Left.GetFName().LexicalLess(Right.GetFName());
		});

		const float AngleStep = 360.0f / static_cast<float>(SecurityControllers.Num());
		for (int32 SlotIndex = 0; SlotIndex < SecurityControllers.Num(); ++SlotIndex)
		{
			AFTSecurityAIController* SecurityController = SecurityControllers[SlotIndex];
			const float SlotAngle = AngleStep * static_cast<float>(SlotIndex);
			const FVector SlotDirection = FVector::ForwardVector.RotateAngleAxis(SlotAngle, FVector::UpVector);
			const FVector DesiredLocation = TargetActor->GetActorLocation() + SlotDirection * EncircleRadius;

			FVector SlotLocation = DesiredLocation;
			FNavLocation ProjectedLocation;
			if (NavigationSystem
				&& NavigationSystem->ProjectPointToNavigation(
					DesiredLocation,
					ProjectedLocation,
					EncircleNavProjectionExtent))
			{
				SlotLocation = ProjectedLocation.Location;
			}

			SecurityController->EncircleSlotLocation = SlotLocation;
			SecurityController->bHasEncircleSlot = true;
		}
	}
}

void UFTSecurityCoordinationComponent::RemoveInvalidControllers()
{
	for (auto ControllerIterator = RegisteredSecurityControllers.CreateIterator(); ControllerIterator; ++ControllerIterator)
	{
		if (!ControllerIterator->IsValid())
		{
			ControllerIterator.RemoveCurrent();
		}
	}
}
