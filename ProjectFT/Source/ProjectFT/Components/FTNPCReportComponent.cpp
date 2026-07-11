#include "FTNPCReportComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/NPC/FTNPCAIController.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

UFTNPCReportComponent::UFTNPCReportComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTNPCReportComponent::EnterReporting()
{
	ResetReportState();

	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	Controller->UpdateTargetState();
	if (Controller->bLogReportDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[NPC] Enter Reporting"));
	}
	BroadcastReportMessage(TAG_FT_Event_NPCReportStarted, Controller->TargetActor, 0.0f, 0.0f);

	if (ShouldCancelReport(Controller))
	{
		CancelReport();
	}
}

bool UFTNPCReportComponent::TickReporting(float DeltaTime)
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return false;
	}

	if (bReportCompleted)
	{
		return true;
	}

	if (bReportCancelled)
	{
		return false;
	}

	Controller->UpdateTargetState();
	if (ShouldCancelReport(Controller))
	{
		CancelReport();
		return false;
	}

	if (!Controller->bIsTargetActivelyStealing && !bObservedShelfDamaged)
	{
		if (CurrentReportProgress > 0.0f)
		{
			ReportElapsedTime = FMath::Max(
				ReportElapsedTime - DeltaTime * (ReportDuration / FMath::Max(ReportDecayDuration, KINDA_SMALL_NUMBER)),
				0.0f
			);
			CurrentReportProgress = FMath::Clamp(ReportElapsedTime / FMath::Max(ReportDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
			BroadcastReportMessage(TAG_FT_Event_NPCReportProgress, Controller->TargetActor, ReportAmount, CurrentReportProgress);

			const int32 ReportPercent = FMath::FloorToInt(CurrentReportProgress * 100.0f);
			if (ReportPercent / 10 < LastLoggedReportDecayPercent / 10)
			{
				LastLoggedReportDecayPercent = ReportPercent;
				if (Controller->bLogReportDebug)
				{
					UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Decay %d%%"), ReportPercent);
				}
			}
		}

		if (CurrentReportProgress <= 0.0f)
		{
			CancelReport();
		}

		return false;
	}

	ReportElapsedTime += DeltaTime;
	CurrentReportProgress = FMath::Clamp(ReportElapsedTime / FMath::Max(ReportDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	BroadcastReportMessage(TAG_FT_Event_NPCReportProgress, Controller->TargetActor, ReportAmount, CurrentReportProgress);
	LastLoggedReportDecayPercent = 101;

	const int32 ReportPercent = FMath::FloorToInt(CurrentReportProgress * 100.0f);
	if (ReportPercent / 10 > LastLoggedReportPercent / 10)
	{
		LastLoggedReportPercent = ReportPercent;
		if (Controller->bLogReportDebug)
		{
			UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Progress %d%%"), ReportPercent);
		}
	}

	if (CurrentReportProgress >= 1.0f)
	{
		CompleteReport();
		return true;
	}

	return false;
}

void UFTNPCReportComponent::CancelReport()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	if (bReportCompleted || bReportCancelled)
	{
		return;
	}

	bReportCancelled = true;
	bObservedShelfDamaged = false;
	CurrentReportProgress = 0.0f;
	ReportElapsedTime = 0.0f;
	LastLoggedReportDecayPercent = 0;
	BroadcastReportMessage(TAG_FT_Event_NPCReportProgress, Controller->TargetActor, ReportAmount, 0.0f);
	if (Controller->bLogReportDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Cancelled"));
	}
}

void UFTNPCReportComponent::HandleReportFlowAvailability(bool bCanStartReportFlow, bool bLogReportDebug)
{
	if (bReportCompleted && !bCanStartReportFlow)
	{
		ResetReportState();
		if (bLogReportDebug)
		{
			UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Rearmed"));
		}
	}

	if (bReportCancelled && bCanStartReportFlow)
	{
		ResetReportState();
		if (bLogReportDebug)
		{
			UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Retry Ready"));
		}
	}
}

void UFTNPCReportComponent::MarkObservedShelfDamage()
{
	bObservedShelfDamaged = true;
}

void UFTNPCReportComponent::HandleStunStateChanged(bool bStunned)
{
	if (bStunned && CurrentReportProgress > 0.0f && !bReportCompleted)
	{
		CancelReport();
	}
}

AFTNPCAIController* UFTNPCReportComponent::GetNPCAIController() const
{
	return Cast<AFTNPCAIController>(GetOwner());
}

bool UFTNPCReportComponent::ShouldCancelReport(const AFTNPCAIController* Controller) const
{
	if (!Controller || Controller->bIsStunned)
	{
		return true;
	}

	if (!Controller->TargetActor)
	{
		return true;
	}

	if (bObservedShelfDamaged)
	{
		return false;
	}

	return !Controller->bHasSeenTarget || Controller->TargetDistance > ReportCancelDistance;
}

void UFTNPCReportComponent::CompleteReport()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	if (bReportCompleted || bReportCancelled)
	{
		return;
	}

	bReportCompleted = true;
	bObservedShelfDamaged = false;
	CurrentReportProgress = 1.0f;

	BroadcastReportMessage(TAG_FT_Event_NPCReportCompleted, Controller->TargetActor, ReportAmount, 1.0f);
	if (Controller->bLogReportDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Completed"));
	}
}

void UFTNPCReportComponent::BroadcastReportMessage(FGameplayTag Channel, AActor* TargetActor, float InReportAmount, float ReportProgress) const
{
	const AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	FFTNPCReportPayloadStruct Payload;
	Payload.ReporterActor = Controller->GetPawn();
	Payload.TargetActor = TargetActor;
	Payload.ReportLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
	Payload.ReportAmount = InReportAmount;
	Payload.ReportProgress = ReportProgress;

	UGameplayMessageSubsystem::Get(Controller).BroadcastMessage(Channel, Payload);
}

void UFTNPCReportComponent::ResetReportState()
{
	ReportElapsedTime = 0.0f;
	CurrentReportProgress = 0.0f;
	bReportCompleted = false;
	bReportCancelled = false;
	LastLoggedReportPercent = -1;
	LastLoggedReportDecayPercent = 101;
}
