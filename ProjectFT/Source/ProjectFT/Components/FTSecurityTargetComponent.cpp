#include "FTSecurityTargetComponent.h"

#include "Engine/World.h"
#include "ProjectFT/Security/FTSecurityAIController.h"

UFTSecurityTargetComponent::UFTSecurityTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTSecurityTargetComponent::ResetTargetMemory()
{
	LastTargetVisibleTime = -BIG_NUMBER;
	bTargetCurrentlyVisible = false;
}

void UFTSecurityTargetComponent::UpdateTargetState(
	const AFTSecurityAIController* Controller,
	AActor* TargetActor,
	bool bInTargetCurrentlyVisible,
	float AttackRange,
	float TargetSightLostGracePeriod,
	float& OutTargetDistance,
	bool& bOutHasSeenTarget,
	bool& bOutIsTargetInAttackRange)
{
	OutTargetDistance = 0.0f;
	bOutHasSeenTarget = false;
	bOutIsTargetInAttackRange = false;
	bTargetCurrentlyVisible = false;

	const APawn* ControlledPawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Controller || !ControlledPawn || !TargetActor)
	{
		ResetTargetMemory();
		return;
	}

	OutTargetDistance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	bTargetCurrentlyVisible = bInTargetCurrentlyVisible;
	if (bTargetCurrentlyVisible)
	{
		LastTargetVisibleTime = Controller->GetWorld()->GetTimeSeconds();
	}

	const float TimeSinceTargetVisible = Controller->GetWorld()->GetTimeSeconds() - LastTargetVisibleTime;
	bOutHasSeenTarget = bTargetCurrentlyVisible || TimeSinceTargetVisible <= TargetSightLostGracePeriod;
	bOutIsTargetInAttackRange = bOutHasSeenTarget && OutTargetDistance <= AttackRange;
}

bool UFTSecurityTargetComponent::WasTargetCurrentlyVisible() const
{
	return bTargetCurrentlyVisible;
}
