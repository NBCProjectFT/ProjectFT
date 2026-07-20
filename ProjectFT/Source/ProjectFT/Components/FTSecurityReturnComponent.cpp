#include "FTSecurityReturnComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Security/FTSecurityAIController.h"
#include "ProjectFT/Security/FTSecurityCharacter.h"
#include "ProjectFT/Struct/FTSecurityResponsePayloadStruct.h"

UFTSecurityReturnComponent::UFTSecurityReturnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTSecurityReturnComponent::InitializeHome(
	AFTSecurityAIController* Controller,
	APawn* ControlledPawn) const
{
	if (!Controller || !ControlledPawn || Controller->bSpawnedFromSecurityRoom)
	{
		return;
	}

	Controller->HomeLocation = ControlledPawn->GetActorLocation();
	Controller->ReturnLocation = Controller->HomeLocation;
	Controller->HomeRotation = ControlledPawn->GetActorRotation();
}

void UFTSecurityReturnComponent::UpdateReturnCollision(AFTSecurityAIController* Controller) const
{
	if (!Controller
		|| !Controller->bReturning
		|| !Controller->bSpawnedFromSecurityRoom
		|| Controller->bReturnCollisionIgnored)
	{
		return;
	}

	AFTSecurityCharacter* SecurityCharacter = Cast<AFTSecurityCharacter>(Controller->GetPawn());
	if (!SecurityCharacter)
	{
		return;
	}

	if (FVector::DistSquared(SecurityCharacter->GetActorLocation(), Controller->ReturnLocation)
		> FMath::Square(Controller->ReturnCollisionIgnoreDistance))
	{
		return;
	}

	Controller->bReturnCollisionIgnored = true;
	SecurityCharacter->IgnorePawnCollisionForDuration(0.0f);
}

void UFTSecurityReturnComponent::HandleMoveCompleted(
	AFTSecurityAIController* Controller,
	const FPathFollowingResult& Result) const
{
	if (!Controller || !Controller->bReturning)
	{
		return;
	}

	// 추격 또는 EQS 이동을 중단한 결과는 복귀 이동 실패가 아니다.
	if (Result.Code == EPathFollowingResult::Aborted)
	{
		return;
	}

	if (!Result.IsSuccess())
	{
		if (!Controller->bReturnFailureLogged)
		{
			Controller->bReturnFailureLogged = true;
			UE_LOG(LogFTSecurity, Warning, TEXT("Security AI '%s' failed to return"), *Controller->GetName());
		}
		return;
	}

	const APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const float DistanceToReturnLocation = FVector::Dist2D(
		ControlledPawn->GetActorLocation(),
		Controller->ReturnLocation);
	if (DistanceToReturnLocation > Controller->ReturnCompletionDistance)
	{
		if (!Controller->bReturnFailureLogged)
		{
			Controller->bReturnFailureLogged = true;
			UE_LOG(
				LogFTSecurity,
				Warning,
				TEXT("Security AI '%s' completed an unrelated move while returning: Distance=%.1f"),
				*Controller->GetName(),
				DistanceToReturnLocation);
		}
		return;
	}

	Controller->bReturnFailureLogged = false;
	CompleteReturn(Controller);
}

void UFTSecurityReturnComponent::CompleteReturn(AFTSecurityAIController* Controller) const
{
	if (!Controller)
	{
		return;
	}

	Controller->bReturning = false;
	Controller->bReturnRequested = false;
	APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (!Controller->bSpawnedFromSecurityRoom)
	{
		ControlledPawn->SetActorRotation(Controller->HomeRotation);
		Controller->SetControlRotation(Controller->HomeRotation);

		if (Controller->bLogSecurityEventDebug)
		{
			UE_LOG(LogFTSecurity, Log, TEXT("Security AI '%s' returned home"), *Controller->GetName());
		}
		return;
	}

	FFTSecurityResponsePayloadStruct Payload;
	Payload.SecurityActor = ControlledPawn;
	Payload.SecurityRoomActor = Controller->SecurityRoomActor;
	Payload.ReturnLocation = Controller->ReturnLocation;

	if (Controller->bLogSecurityEventDebug)
	{
		UE_LOG(LogFTSecurity, Log, TEXT("Security AI '%s' returned to security room"), *Controller->GetName());
	}
	UGameplayMessageSubsystem::Get(Controller).BroadcastMessage(TAG_FT_Event_SecurityReturnedToRoom, Payload);
}
