#include "FTSecurityResponseComponent.h"

#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "ProjectFT/Components/FTSecurityCallComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Security/FTSecurityAIController.h"
#include "ProjectFT/Security/FTSecurityCharacter.h"
#include "ProjectFT/Struct/FTCharacterAttackedPayloadStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

UFTSecurityResponseComponent::UFTSecurityResponseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTSecurityResponseComponent::HandleSecurityCalled(
	AFTSecurityAIController* Controller,
	const FFTNPCReportPayloadStruct& Payload) const
{
	if (!Controller || Controller->bTargetCaptured)
	{
		return;
	}

	if (!Payload.TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: TargetActor is null"));
		return;
	}

	const APawn* ControlledPawn = Controller->GetPawn();
	if (Payload.ReporterActor == ControlledPawn)
	{
		return;
	}

	if (!ControlledPawn || !Controller->SightConfig)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: ControlledPawn or SightConfig is null"));
		return;
	}

	if (Controller->bReturning)
	{
		Controller->StopMovement();
	}
	if (Controller->SecurityCallComponent)
	{
		Controller->SecurityCallComponent->StopSecurityCall();
	}
	Controller->bCanRequestSecuritySupport = false;
	Controller->bReturning = false;
	Controller->bReturnRequested = false;
	Controller->bInvestigateRequested = false;
	Controller->bStunRequested = false;
	Controller->bReturnFailureLogged = false;
	Controller->bReturnCollisionIgnored = false;
	if (AFTSecurityCharacter* SecurityCharacter = Cast<AFTSecurityCharacter>(Controller->GetPawn()))
	{
		SecurityCharacter->RestorePawnCollision();
	}
	Controller->bSecurityCalled = true;
	Controller->SetTargetActor(Payload.TargetActor);
	Controller->InvestigateLocation = Payload.ReportLocation.IsNearlyZero()
		? Payload.TargetActor->GetActorLocation()
		: Payload.ReportLocation;
	Controller->UpdateTargetState();
}

void UFTSecurityResponseComponent::HandleShelfDamaged(
	AFTSecurityAIController* Controller,
	const FFTMessagePayloadStruct& Payload) const
{
	if (!Controller || Controller->bTargetCaptured || Controller->bIsStunned)
	{
		return;
	}

	AActor* SuspectActor = Controller->ResolvePlayerActor(Payload.InstigatorActor);
	AActor* DamagedShelf = Payload.TargetActor;
	if (!SuspectActor)
	{
		// TODO: 공격 측에서 DamageCauser를 정상 전달하면 싱글플레이용 fallback을 제거한다.
		SuspectActor = UGameplayStatics::GetPlayerPawn(Controller, 0);
	}

	if (!SuspectActor || !DamagedShelf)
	{
		if (Controller->bLogSecurityEventDebug)
		{
			UE_LOG(
				LogFTSecurity,
				Warning,
				TEXT("Security AI ignored shelf damage: Instigator=%s Player=%s Shelf=%s"),
				*GetNameSafe(Payload.InstigatorActor),
				*GetNameSafe(SuspectActor),
				*GetNameSafe(DamagedShelf));
		}
		return;
	}

	Controller->SetTargetActor(SuspectActor);
	const bool bCanSeePlayer = Controller->IsTargetCurrentlyVisible();
	const bool bCanSeeDamagedShelf = Controller->LineOfSightTo(DamagedShelf);
	if (!bCanSeePlayer || !bCanSeeDamagedShelf)
	{
		return;
	}

	Controller->bReturning = false;
	Controller->bReturnRequested = false;
	Controller->bSecurityCalled = true;
	Controller->bCanRequestSecuritySupport = true;
	if (Controller->SecurityCallComponent)
	{
		Controller->SecurityCallComponent->StartSecurityCall(SuspectActor);
	}
	Controller->InvestigateLocation = SuspectActor->GetActorLocation();
	Controller->UpdateTargetState();

	if (Controller->bLogSecurityEventDebug)
	{
		UE_LOG(
			LogFTSecurity,
			Log,
			TEXT("Security AI '%s' witnessed shelf damage, chasing %s"),
			*Controller->GetName(),
			*GetNameSafe(SuspectActor));
	}
}

void UFTSecurityResponseComponent::HandleCharacterAttacked(
	AFTSecurityAIController* Controller,
	const FFTCharacterAttackedPayloadStruct& Payload) const
{
	if (!Controller || Controller->bTargetCaptured || Controller->bIsStunned)
	{
		return;
	}

	AActor* SuspectActor = Controller->ResolvePlayerActor(Payload.InstigatorActor);
	if (!Controller->IsPlayerActor(SuspectActor))
	{
		return;
	}

	const bool bAttackedSelf = Payload.TargetActor == Controller->GetPawn();
	const bool bWitnessedAssault = !bAttackedSelf && Cast<AFTAICharacterBase>(Payload.TargetActor);
	if (!bAttackedSelf && !bWitnessedAssault)
	{
		return;
	}

	Controller->SetTargetActor(SuspectActor);
	if (bWitnessedAssault)
	{
		const bool bCanSeePlayer = Controller->IsTargetCurrentlyVisible();
		const bool bCanSeeDamagedActor = Controller->LineOfSightTo(Payload.TargetActor);
		if (!bCanSeePlayer || !bCanSeeDamagedActor)
		{
			return;
		}
	}

	const bool bShouldStartSecuritySupportCall = !Controller->bSecurityCalled && !Controller->bSecurityChaseActive;

	Controller->StopMovement();
	Controller->bReturning = false;
	Controller->bReturnRequested = false;
	Controller->bInvestigateRequested = false;
	Controller->bStunRequested = false;
	Controller->bReturnFailureLogged = false;
	Controller->bReturnCollisionIgnored = false;
	Controller->bSecurityCalled = true;
	if (bShouldStartSecuritySupportCall)
	{
		Controller->bCanRequestSecuritySupport = true;
		if (Controller->SecurityCallComponent)
		{
			Controller->SecurityCallComponent->StartSecurityCall(SuspectActor);
		}
	}
	Controller->InvestigateLocation = SuspectActor->GetActorLocation();
	Controller->UpdateTargetState();

	if (Controller->bLogSecurityEventDebug)
	{
		if (bAttackedSelf)
		{
			UE_LOG(
				LogFTSecurity,
				Log,
				TEXT("Security AI '%s' attacked by player, chasing %s"),
				*Controller->GetName(),
				*GetNameSafe(SuspectActor)
			);
		}
		else
		{
			UE_LOG(
				LogFTSecurity,
				Log,
				TEXT("Security AI '%s' witnessed assault, chasing %s"),
				*Controller->GetName(),
				*GetNameSafe(SuspectActor)
			);
		}
	}
}
