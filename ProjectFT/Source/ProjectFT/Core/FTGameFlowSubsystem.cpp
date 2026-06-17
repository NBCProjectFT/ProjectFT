#include "FTGameFlowSubsystem.h"

#include "FTGameState.h"
#include "FTLogChannels.h"
#include "../Message/FTGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

void UFTGameFlowSubsystem::RequestStartRaid()
{
	if (CurrentFlowState == EFTFlowStateType::RaidInProgress)
	{
		return;
	}

	SetFlowState(EFTFlowStateType::RaidInProgress);
	BroadcastFlowEvent(TAG_FT_Event_RaidStarted);
}

void UFTGameFlowSubsystem::RequestEscapeRaid()
{
	if (CurrentFlowState != EFTFlowStateType::RaidInProgress && CurrentFlowState != EFTFlowStateType::Escaping)
	{
		return;
	}

	SetFlowState(EFTFlowStateType::Escaping);
	BroadcastFlowEvent(TAG_FT_Event_RaidEscaped);
	EnterSettlement();
}

void UFTGameFlowSubsystem::RequestFailRaid()
{
	if (CurrentFlowState == EFTFlowStateType::Failed || CurrentFlowState == EFTFlowStateType::Settlement)
	{
		return;
	}

	SetFlowState(EFTFlowStateType::Failed);
	BroadcastFlowEvent(TAG_FT_Event_RaidFailed);
}

void UFTGameFlowSubsystem::EnterSettlement()
{
	SetFlowState(EFTFlowStateType::Settlement);
}

void UFTGameFlowSubsystem::ReturnToBase()
{
	SetFlowState(EFTFlowStateType::Base);
}

void UFTGameFlowSubsystem::SetFlowState(EFTFlowStateType NewFlowState)
{
	CurrentFlowState = NewFlowState;

	if (UWorld* World = GetWorld())
	{
		if (AFTGameState* FTGameState = World->GetGameState<AFTGameState>())
		{
			FTGameState->SetFlowState(NewFlowState);
		}
	}

	UE_LOG(LogFTFlow, Log, TEXT("Flow state changed to %d"), static_cast<uint8>(NewFlowState));
}

void UFTGameFlowSubsystem::BroadcastFlowEvent(FGameplayTag Channel) const
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	MessageSubsystem.BroadcastMessage(Channel, Payload);
}
