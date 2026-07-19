#include "FTSecurityPursuitStateComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Components/FTSecurityCallComponent.h"
#include "ProjectFT/Components/FTSecurityTargetComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Security/FTSecurityAIController.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"

UFTSecurityPursuitStateComponent::UFTSecurityPursuitStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTSecurityPursuitStateComponent::UpdateTargetSeenState(AFTSecurityAIController* Controller)
{
	if (!Controller)
	{
		return;
	}

	const bool bShouldReportTargetSeen = Controller->bSecurityCalled && Controller->bHasSeenTarget;
	if (bReportedTargetSeenToChaseGauge == bShouldReportTargetSeen)
	{
		return;
	}

	FFTSecurityChaseGaugePayloadStruct Payload;
	Payload.SecurityActor = Controller->GetPawn();
	Payload.TargetActor = Controller->TargetActor;
	Payload.LastKnownLocation = Controller->InvestigateLocation;
	Payload.bHasSeenTarget = bShouldReportTargetSeen;

	UGameplayMessageSubsystem::Get(Controller).BroadcastMessage(
		bShouldReportTargetSeen ? TAG_FT_Event_SecurityTargetSeen : TAG_FT_Event_SecurityTargetLost,
		Payload
	);
	bReportedTargetSeenToChaseGauge = bShouldReportTargetSeen;
}

void UFTSecurityPursuitStateComponent::HandleChaseGaugeChanged(
	AFTSecurityAIController* Controller,
	const FFTSecurityChaseGaugePayloadStruct& Payload) const
{
	if (!Controller)
	{
		return;
	}

	Controller->SecurityChaseGauge = Payload.ChaseGauge;
	Controller->bSecurityChaseActive = Controller->SecurityChaseGauge > 0.0f;
}

void UFTSecurityPursuitStateComponent::HandleChaseEnded(
	AFTSecurityAIController* Controller,
	const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	if (!Controller || Controller->bTargetCaptured)
	{
		return;
	}

	Controller->StopMovement();
	if (Controller->SecurityCallComponent)
	{
		Controller->SecurityCallComponent->StopSecurityCall();
	}
	Controller->bCanRequestSecuritySupport = false;
	Controller->bReturning = true;
	Controller->bReturnRequested = true;
	Controller->bReturnFailureLogged = false;
	Controller->bReturnCollisionIgnored = false;
	Controller->SecurityChaseGauge = 0.0f;
	Controller->bSecurityChaseActive = false;
	Controller->bSecurityCalled = false;
	Controller->ClearFocus(EAIFocusPriority::Gameplay);
	Controller->TargetActor = nullptr;
	if (Controller->SecurityTargetComponent)
	{
		Controller->SecurityTargetComponent->ResetTargetMemory();
	}
	Controller->TargetDistance = 0.0f;
	Controller->bHasSeenTarget = false;
	Controller->bIsTargetInAttackRange = false;
	bReportedTargetSeenToChaseGauge = false;

	if (Controller->bLogSecurityEventDebug)
	{
		UE_LOG(
			LogFTSecurity,
			Log,
			TEXT("Security AI '%s' requested return to %s"),
			*Controller->GetName(),
			*Controller->ReturnLocation.ToString());
	}
}

void UFTSecurityPursuitStateComponent::ClearReportedTargetSeen(AFTSecurityAIController* Controller)
{
	if (!bReportedTargetSeenToChaseGauge)
	{
		return;
	}

	if (Controller)
	{
		Controller->bHasSeenTarget = false;
		UpdateTargetSeenState(Controller);
	}

	bReportedTargetSeenToChaseGauge = false;
}
