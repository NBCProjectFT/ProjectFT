#include "FTGameFlowSubsystem.h"

#include "FTGameState.h"
#include "FTLogChannels.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/Data/FTGameDataAsset.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

namespace
{
	const FName FallbackLoadingLevelName(TEXT("Lvl_Loading"));
	const FName FallbackMainMenuLevelName(TEXT("Lvl_MainMenu"));
	const FName FallbackBaseLevelName(TEXT("Lvl_Hub"));
	const FName FallbackRaidLevelName(TEXT("Lvl_Main"));
}

void UFTGameFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Event_UI_MainMenu_StartGame, this, &ThisClass::HandleStartGameMessage));
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Request_Flow_StartGame, this, &ThisClass::HandleFlowRequestMessage));
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Request_Flow_StartRaid, this, &ThisClass::HandleFlowRequestMessage));
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Request_Flow_StartEscape, this, &ThisClass::HandleFlowRequestMessage));
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Request_Flow_CancelEscape, this, &ThisClass::HandleFlowRequestMessage));
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Request_Flow_CompleteEscape, this, &ThisClass::HandleFlowRequestMessage));
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Request_Flow_FailRaid, this, &ThisClass::HandleFlowRequestMessage));
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Request_Flow_ReturnToBase, this, &ThisClass::HandleFlowRequestMessage));
}

void UFTGameFlowSubsystem::Deinitialize()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	for (FGameplayMessageListenerHandle& ListenerHandle : FlowRequestListenerHandles)
	{
		if (ListenerHandle.IsValid())
		{
			MessageSubsystem.UnregisterListener(ListenerHandle);
		}
	}
	FlowRequestListenerHandles.Reset();

	Super::Deinitialize();
}

void UFTGameFlowSubsystem::HandleStartGameMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	UE_LOG(LogFTFlow, Log, TEXT("Start game message received. Channel=%s Instigator=%s"),
		*Channel.ToString(),
		*GetNameSafe(Payload.InstigatorActor));

	RequestStartGame();
}

void UFTGameFlowSubsystem::HandleFlowRequestMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload)
{
	UE_LOG(LogFTFlow, Log, TEXT("Flow request received. Channel=%s Instigator=%s"),
		*Channel.ToString(),
		*GetNameSafe(Payload.InstigatorActor));

	if (Channel == TAG_FT_Request_Flow_StartGame)
	{
		RequestStartGame();
	}
	else if (Channel == TAG_FT_Request_Flow_StartRaid)
	{
		RequestStartRaid();
	}
	else if (Channel == TAG_FT_Request_Flow_StartEscape)
	{
		RequestEscapeRaid();
	}
	else if (Channel == TAG_FT_Request_Flow_CancelEscape)
	{
		RequestCancelEscape();
	}
	else if (Channel == TAG_FT_Request_Flow_CompleteEscape)
	{
		RequestCompleteEscape();
	}
	else if (Channel == TAG_FT_Request_Flow_FailRaid)
	{
		RequestFailRaid();
	}
	else if (Channel == TAG_FT_Request_Flow_ReturnToBase)
	{
		ReturnToBase();
	}
}

void UFTGameFlowSubsystem::RequestStartGame()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->HideMainMenu(/*bKeepMouseCursor=*/true);
		}
	}

	TravelToState(EFTFlowStateType::Base);
}

void UFTGameFlowSubsystem::RequestStartRaid()
{
	if (CurrentFlowState == EFTFlowStateType::RaidEntering || CurrentFlowState == EFTFlowStateType::RaidInProgress)
	{
		return;
	}

	TravelToState(EFTFlowStateType::RaidEntering);
}

void UFTGameFlowSubsystem::RequestEscapeRaid()
{
	if (CurrentFlowState != EFTFlowStateType::RaidInProgress && CurrentFlowState != EFTFlowStateType::Escaping)
	{
		return;
	}

	SetFlowState(EFTFlowStateType::Escaping);
}

void UFTGameFlowSubsystem::RequestCancelEscape()
{
	if (CurrentFlowState != EFTFlowStateType::Escaping)
	{
		return;
	}

	SetFlowState(EFTFlowStateType::RaidInProgress);
}

void UFTGameFlowSubsystem::RequestCompleteEscape()
{
	if (CurrentFlowState != EFTFlowStateType::Escaping)
	{
		return;
	}

	SetFlowState(EFTFlowStateType::Escaped);
}

void UFTGameFlowSubsystem::RequestFailRaid()
{
	if (CurrentFlowState == EFTFlowStateType::Failed)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
			{
				UIManager->ShowFailScreen();
			}
		}
		return;
	}

	SetFlowState(EFTFlowStateType::Failed);
}

void UFTGameFlowSubsystem::ReturnToBase()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->HideEscapedRaid();
			UIManager->HideFailScreen();
		}
	}

	TravelToState(EFTFlowStateType::Base);
}

void UFTGameFlowSubsystem::CompleteLoadingAndOpenCurrentStateLevel()
{
	const FName TargetLevelName = ResolveLevelNameForState(CurrentFlowState);
	if (TargetLevelName.IsNone())
	{
		UE_LOG(LogFTFlow, Error, TEXT("Loading completed but no level route exists. CurrentFlowState=%d"),
			static_cast<uint8>(CurrentFlowState));
		return;
	}

	// RaidEntering은 Main 레벨 진입을 위한 이동 상태다. 실제 레벨을 열기 직전에 플레이 상태로 확정한다.
	if (CurrentFlowState == EFTFlowStateType::RaidEntering)
	{
		SetFlowState(EFTFlowStateType::RaidInProgress);
	}

	UE_LOG(LogFTFlow, Log, TEXT("Loading completed. Opening state level. State=%d Target=%s"),
		static_cast<uint8>(CurrentFlowState),
		*TargetLevelName.ToString());

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->bShowMouseCursor = false;
	}

	OpenLevelByName(TargetLevelName);
}

void UFTGameFlowSubsystem::PreloadCurrentStateAssetsAsync(FSimpleDelegate OnLoaded, FFTAssetLoadProgressDelegate OnProgress) const
{
	// LoadingGameMode calls this after the flow has selected the active GameData.
	// MainMenu -> StartGame switches the active GameData to DA_HubData, so this loads DA_HubData's preload range.
	UFTAssetManager::Get().PreloadGameDataAssetsAsync(OnLoaded, OnProgress);
}

void UFTGameFlowSubsystem::TravelToState(EFTFlowStateType TargetFlowState)
{
	if (ShouldUseLoadingForState(TargetFlowState))
	{
		TravelToStateWithLoading(TargetFlowState);
		return;
	}

	SetFlowState(TargetFlowState);
	OpenLevelByName(ResolveLevelNameForState(TargetFlowState));
}

void UFTGameFlowSubsystem::TravelToStateWithLoading(EFTFlowStateType TargetFlowState)
{
	SetFlowState(TargetFlowState);
	OpenLevelByName(ResolveLoadingLevelName());
}

const FFTFlowStateDefinition* UFTGameFlowSubsystem::FindFlowStateDefinition(EFTFlowStateType State) const
{
	const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData();
	if (!GameData)
	{
		return nullptr;
	}

	for (const FFTFlowStateDefinition& Definition : GameData->FlowStateDefinitions)
	{
		if (Definition.State == State)
		{
			return &Definition;
		}
	}

	return nullptr;
}

FName UFTGameFlowSubsystem::ResolveLoadingLevelName() const
{
	if (const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData())
	{
		if (!GameData->LoadingLevelName.IsNone())
		{
			return GameData->LoadingLevelName;
		}
	}

	return FallbackLoadingLevelName;
}

FName UFTGameFlowSubsystem::ResolveLevelNameForState(EFTFlowStateType State) const
{
	if (const FFTFlowStateDefinition* Definition = FindFlowStateDefinition(State))
	{
		if (!Definition->TargetLevelName.IsNone())
		{
			return Definition->TargetLevelName;
		}
	}

	switch (State)
	{
	case EFTFlowStateType::MainMenu:
		return FallbackMainMenuLevelName;
	case EFTFlowStateType::Base:
		return FallbackBaseLevelName;
	case EFTFlowStateType::RaidEntering:
	case EFTFlowStateType::RaidInProgress:
	case EFTFlowStateType::Escaping:
	case EFTFlowStateType::Escaped:
	case EFTFlowStateType::Failed:
		return FallbackRaidLevelName;
	default:
		return NAME_None;
	}
}

bool UFTGameFlowSubsystem::ShouldUseLoadingForState(EFTFlowStateType State) const
{
	if (const FFTFlowStateDefinition* Definition = FindFlowStateDefinition(State))
	{
		return Definition->bUseLoadingLevel;
	}

	return State == EFTFlowStateType::Base || State == EFTFlowStateType::RaidEntering;
}

void UFTGameFlowSubsystem::OpenLevelByName(FName LevelName) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFTFlow, Error, TEXT("OpenLevel failed because World is null. Target=%s"), *LevelName.ToString());
		return;
	}

	UE_LOG(LogFTFlow, Log, TEXT("Opening level. FromMap=%s Target=%s FlowState=%d AuthGameMode=%s"),
		*World->GetMapName(),
		*LevelName.ToString(),
		static_cast<uint8>(CurrentFlowState),
		*GetNameSafe(World->GetAuthGameMode()));

	RestoreMenuInputBeforeTravel(LevelName);
	UGameplayStatics::OpenLevel(World, LevelName, true);
}

void UFTGameFlowSubsystem::RestoreMenuInputBeforeTravel(FName LevelName) const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->HideMainMenu(/*bKeepMouseCursor=*/LevelName == ResolveLoadingLevelName());
		}
	}
}

void UFTGameFlowSubsystem::SetFlowState(EFTFlowStateType NewFlowState)
{
	if (CurrentFlowState == NewFlowState)
	{
		return;
	}

	CurrentFlowState = NewFlowState;

	if (UWorld* World = GetWorld())
	{
		if (AFTGameState* FTGameState = World->GetGameState<AFTGameState>())
		{
			FTGameState->SetFlowState(NewFlowState);
		}
	}

	UE_LOG(LogFTFlow, Log, TEXT("Flow state changed to %d"), static_cast<uint8>(NewFlowState));
	BroadcastFlowStateChanged();
	HandleFlowStateEntered(NewFlowState);
}

void UFTGameFlowSubsystem::HandleFlowStateEntered(EFTFlowStateType NewFlowState)
{
	switch (NewFlowState)
	{
	case EFTFlowStateType::RaidInProgress:
		BroadcastFlowEvent(TAG_FT_Event_RaidStarted);
		break;
	case EFTFlowStateType::Escaped:
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
			{
				UIManager->ShowEscapedRaid();
			}
		}
		BroadcastFlowEvent(TAG_FT_Event_RaidEscaped);
		break;
	case EFTFlowStateType::Failed:
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
			{
				UIManager->ShowFailScreen();
			}
		}
		BroadcastFlowEvent(TAG_FT_Event_RaidFailed);
		break;
	default:
		break;
	}
}

void UFTGameFlowSubsystem::BroadcastFlowStateChanged() const
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	Payload.Value = static_cast<float>(static_cast<uint8>(CurrentFlowState));
	MessageSubsystem.BroadcastMessage(TAG_FT_Event_FlowStateChanged, Payload);
}

void UFTGameFlowSubsystem::BroadcastFlowEvent(FGameplayTag Channel) const
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	MessageSubsystem.BroadcastMessage(Channel, Payload);
}
