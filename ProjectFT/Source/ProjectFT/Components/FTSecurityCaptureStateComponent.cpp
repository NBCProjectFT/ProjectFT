#include "FTSecurityCaptureStateComponent.h"

#include "ProjectFT/Components/FTSecurityCallComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Security/FTSecurityAIController.h"
#include "ProjectFT/Security/FTSecurityCharacter.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

UFTSecurityCaptureStateComponent::UFTSecurityCaptureStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTSecurityCaptureStateComponent::HandleTargetCaptured(
	AFTSecurityAIController* Controller,
	const FFTNPCReportPayloadStruct& Payload) const
{
	if (!Controller || !Payload.ReporterActor || !Payload.TargetActor || !Controller->GetPawn())
	{
		return;
	}

	Controller->StopMovement();
	if (Controller->SecurityCallComponent)
	{
		Controller->SecurityCallComponent->StopSecurityCall();
	}
	Controller->bCanRequestSecuritySupport = false;
	Controller->TargetActor = Payload.TargetActor;
	Controller->InvestigateLocation = Payload.ReportLocation;
	Controller->bTargetCaptured = true;
	Controller->bIsCaptor = Payload.ReporterActor == Controller->GetPawn();
	Controller->bIsTargetCapturedByOtherSecurity = !Controller->bIsCaptor;
	Controller->bReturnRequested = !Controller->bIsCaptor;
	Controller->bReturning = !Controller->bIsCaptor;
	Controller->bInvestigateRequested = false;
	Controller->bStunRequested = false;
	Controller->bSecurityCalled = false;
	Controller->bHasSeenTarget = false;
	Controller->bIsTargetInAttackRange = false;
	Controller->UpdateChaseGaugeTargetSeenState();

	if (Controller->bLogSecurityEventDebug)
	{
		UE_LOG(
			LogFTSecurity,
			Log,
			TEXT("Security AI '%s' received target captured: Captor=%s IsCaptor=%s"),
			*Controller->GetName(),
			*GetNameSafe(Payload.ReporterActor),
			Controller->bIsCaptor ? TEXT("true") : TEXT("false")
		);
	}
}

void UFTSecurityCaptureStateComponent::HandleTargetEscaped(
	AFTSecurityAIController* Controller,
	const FFTNPCReportPayloadStruct& Payload) const
{
	if (!Controller || !Payload.ReporterActor || !Payload.TargetActor)
	{
		return;
	}

	const bool bWasEscapedFromThisSecurity = Payload.ReporterActor == Controller->GetPawn();
	Controller->StopMovement();
	Controller->TargetActor = Payload.TargetActor;
	Controller->InvestigateLocation = Payload.ReportLocation.IsNearlyZero()
		? Payload.TargetActor->GetActorLocation()
		: Payload.ReportLocation;
	Controller->bTargetCaptured = false;
	Controller->bIsCaptor = false;
	Controller->bIsTargetCapturedByOtherSecurity = false;
	Controller->bReturnRequested = false;
	Controller->bReturning = false;
	Controller->bInvestigateRequested = true;
	Controller->bStunRequested = bWasEscapedFromThisSecurity;
	Controller->bSecurityCalled = true;
	Controller->bCanRequestSecuritySupport = false;
	if (Controller->SecurityCallComponent)
	{
		Controller->SecurityCallComponent->StopSecurityCall();
	}
	Controller->bReturnFailureLogged = false;
	Controller->bReturnCollisionIgnored = false;

	if (AFTSecurityCharacter* SecurityCharacter = Cast<AFTSecurityCharacter>(Controller->GetPawn()))
	{
		SecurityCharacter->RestorePawnCollision();
	}

	Controller->UpdateTargetState();
	if (Controller->bLogSecurityEventDebug)
	{
		UE_LOG(
			LogFTSecurity,
			Log,
			TEXT("Security AI '%s' received target escaped: StunRequested=%s Location=%s"),
			*Controller->GetName(),
			Controller->bStunRequested ? TEXT("true") : TEXT("false"),
			*Controller->InvestigateLocation.ToString()
		);
	}
}
