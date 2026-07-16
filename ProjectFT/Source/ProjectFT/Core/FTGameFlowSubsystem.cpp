#include "FTGameFlowSubsystem.h"

#include "FTGameState.h"
#include "FTLogChannels.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "ProjectFT/Data/FTGameDataAsset.h"
#include "ProjectFT/Data/FTLevelPreloadDataAsset.h"
#include "ProjectFT/Core/FTSaveSubsystem.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTFlowLevelRouteStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

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
	FlowRequestListenerHandles.Add(MessageSubsystem.RegisterListener(TAG_FT_Request_Flow_ReturnToMainMenu, this, &ThisClass::HandleFlowRequestMessage));
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
	else if (Channel == TAG_FT_Request_Flow_ReturnToMainMenu)
	{
		ReturnToMainMenu();
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
	RequestStartRaidAtLevel(ResolveLevelNameForState(EFTFlowStateType::RaidEntering));
}

bool UFTGameFlowSubsystem::RequestStartRaidAtLevel(const FName TargetLevelName)
{
	if (!CanStartRaidAtLevel(TargetLevelName))
	{
		return false;
	}

	PendingRaidLevelName = TargetLevelName;
	TravelToState(EFTFlowStateType::RaidEntering, TargetLevelName);
	return true;
}

bool UFTGameFlowSubsystem::CanStartRaidAtLevel(const FName TargetLevelName) const
{
	return !TargetLevelName.IsNone()
		&& CurrentFlowState != EFTFlowStateType::RaidEntering
		&& CurrentFlowState != EFTFlowStateType::RaidInProgress
		&& CurrentFlowState != EFTFlowStateType::Escaping;
}

void UFTGameFlowSubsystem::RequestEscapeRaid()
{
	if (CurrentFlowState != EFTFlowStateType::RaidInProgress && CurrentFlowState != EFTFlowStateType::Escaping)
	{
		UE_LOG(LogFTFlow, Warning, TEXT("Start escape request ignored because current flow state is not raid. CurrentFlowState=%d CurrentLevel=%s"),
			static_cast<uint8>(CurrentFlowState),
			*ResolveCurrentWorldLevelName().ToString());
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
		UE_LOG(LogFTFlow, Warning, TEXT("Complete escape request ignored because current flow state is not escaping. CurrentFlowState=%d CurrentLevel=%s"),
			static_cast<uint8>(CurrentFlowState),
			*ResolveCurrentWorldLevelName().ToString());
		return;
	}

	UE_LOG(LogFTFlow, Log, TEXT("Escape completed. Entering escaped flow state."));
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
	PendingRaidLevelName = NAME_None;

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

void UFTGameFlowSubsystem::ReturnToMainMenu()
{
	PendingRaidLevelName = NAME_None;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UFTUIManagerSubsystem>())
		{
			UIManager->HideEscapedRaid();
			UIManager->HideFailScreen();
			UIManager->HidePauseMenu();
		}
	}

	TravelToState(EFTFlowStateType::MainMenu);
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
	const TSoftObjectPtr<UFTLevelPreloadDataAsset> LevelPreloadDataAsset = GetCurrentStateLevelPreloadDataAsset();
	if (LevelPreloadDataAsset.IsNull())
	{
		UE_LOG(LogFTFlow, Warning, TEXT("Preload has no level preload data candidate. State=%d"),
			static_cast<uint8>(CurrentFlowState));
	}
	else
	{
		UE_LOG(LogFTFlow, Log, TEXT("Preloading current state assets. State=%d DataAsset=%s"),
			static_cast<uint8>(CurrentFlowState),
			*LevelPreloadDataAsset.ToSoftObjectPath().ToString());
	}

	UFTAssetManager::Get().PreloadLevelAssetsAsync(LevelPreloadDataAsset, OnLoaded, OnProgress);
}

void UFTGameFlowSubsystem::SyncFlowStateWithCurrentLevel()
{
	const EFTFlowStateType ResolvedFlowState = ResolveFlowStateForCurrentWorld();
	if (ResolvedFlowState == CurrentFlowState)
	{
		if (UWorld* World = GetWorld())
		{
			if (AFTGameState* FTGameState = World->GetGameState<AFTGameState>())
			{
				FTGameState->SetFlowState(CurrentFlowState);
			}
		}
		return;
	}

	UE_LOG(LogFTFlow, Log, TEXT("Syncing flow state from current level. Level=%s From=%d To=%d"),
		*ResolveCurrentWorldLevelName().ToString(),
		static_cast<uint8>(CurrentFlowState),
		static_cast<uint8>(ResolvedFlowState));

	SetFlowState(ResolvedFlowState);
}

TSoftObjectPtr<UFTLevelPreloadDataAsset> UFTGameFlowSubsystem::GetLevelPreloadDataAssetForState(EFTFlowStateType State) const
{
	if (const FFTFlowLevelRouteStruct* Route = FindFlowLevelRouteByState(State))
	{
		if (!Route->LevelPreloadDataAsset.IsNull())
		{
			return Route->LevelPreloadDataAsset;
		}
	}

	const FName TargetLevelName = ResolveLevelNameForState(State);
	if (TargetLevelName.IsNone())
	{
		return TSoftObjectPtr<UFTLevelPreloadDataAsset>();
	}

	const FString AssetName = FString::Printf(TEXT("DA_LevelPreload_%s"), *TargetLevelName.ToString());
	const FString ObjectPath = FString::Printf(TEXT("/Game/Blueprints/LevelPreload/%s.%s"), *AssetName, *AssetName);
	return TSoftObjectPtr<UFTLevelPreloadDataAsset>(FSoftObjectPath(ObjectPath));
}

TSoftObjectPtr<UFTLevelPreloadDataAsset> UFTGameFlowSubsystem::GetCurrentStateLevelPreloadDataAsset() const
{
	return GetLevelPreloadDataAssetForState(CurrentFlowState);
}

TSoftObjectPtr<UFTLevelPreloadDataAsset> UFTGameFlowSubsystem::ResolveLevelPreloadDataAssetForCurrentState() const
{
	if (const FFTFlowLevelRouteStruct* Route = FindFlowLevelRouteByState(CurrentFlowState))
	{
		if (!Route->LevelPreloadDataAsset.IsNull())
		{
			return Route->LevelPreloadDataAsset;
		}
	}

	FName LevelName = ResolveCurrentWorldLevelName();
	if (LevelName.IsNone() || LevelName == ResolveLoadingLevelName())
	{
		LevelName = ResolveLevelNameForState(CurrentFlowState);
	}

	if (LevelName.IsNone())
	{
		return TSoftObjectPtr<UFTLevelPreloadDataAsset>();
	}

	const FString AssetName = FString::Printf(TEXT("DA_LevelPreload_%s"), *LevelName.ToString());
	const FString ObjectPath = FString::Printf(TEXT("/Game/Blueprints/LevelPreload/%s.%s"), *AssetName, *AssetName);
	return TSoftObjectPtr<UFTLevelPreloadDataAsset>(FSoftObjectPath(ObjectPath));
}

void UFTGameFlowSubsystem::TravelToState(EFTFlowStateType TargetFlowState, FName RequestedLevelName)
{
	const FName TargetLevelName = RequestedLevelName.IsNone()
		? ResolveLevelNameForState(TargetFlowState)
		: RequestedLevelName;

	if (ShouldUseLoadingForState(TargetFlowState, TargetLevelName))
	{
		TravelToStateWithLoading(TargetFlowState);
		return;
	}

	SetFlowState(TargetFlowState);
	OpenLevelByName(TargetLevelName);
}

void UFTGameFlowSubsystem::TravelToStateWithLoading(EFTFlowStateType TargetFlowState)
{
	SetFlowState(TargetFlowState);
	OpenLevelByName(ResolveLoadingLevelName());
}

const FFTFlowLevelRouteStruct* UFTGameFlowSubsystem::FindFlowLevelRouteByState(EFTFlowStateType State) const
{
	const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData();
	if (!GameData || GameData->FlowLevelRouteDataTable.IsNull())
	{
		return nullptr;
	}

	const UDataTable* RouteTable = UFTAssetManager::GetAsset(GameData->FlowLevelRouteDataTable);
	if (!RouteTable)
	{
		UE_LOG(LogFTFlow, Warning, TEXT("Flow route data table failed to load: %s"),
			*GameData->FlowLevelRouteDataTable.ToSoftObjectPath().ToString());
		return nullptr;
	}

	TArray<FFTFlowLevelRouteStruct*> Routes;
	RouteTable->GetAllRows(TEXT("FTGameFlowSubsystem.FindFlowLevelRouteByState"), Routes);
	for (const FFTFlowLevelRouteStruct* Route : Routes)
	{
		if (Route && Route->State == State)
		{
			return Route;
		}
	}

	return nullptr;
}

const FFTFlowLevelRouteStruct* UFTGameFlowSubsystem::FindFlowLevelRouteByLevelName(FName LevelName) const
{
	if (LevelName.IsNone())
	{
		return nullptr;
	}

	const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData();
	if (!GameData || GameData->FlowLevelRouteDataTable.IsNull())
	{
		return nullptr;
	}

	const UDataTable* RouteTable = UFTAssetManager::GetAsset(GameData->FlowLevelRouteDataTable);
	if (!RouteTable)
	{
		UE_LOG(LogFTFlow, Warning, TEXT("Flow route data table failed to load: %s"),
			*GameData->FlowLevelRouteDataTable.ToSoftObjectPath().ToString());
		return nullptr;
	}

	TArray<FFTFlowLevelRouteStruct*> Routes;
	RouteTable->GetAllRows(TEXT("FTGameFlowSubsystem.FindFlowLevelRouteByLevelName"), Routes);
	for (const FFTFlowLevelRouteStruct* Route : Routes)
	{
		if (!Route || Route->Level.IsNull())
		{
			continue;
		}

		const FSoftObjectPath LevelPath = Route->Level.ToSoftObjectPath();
		const FName RouteLevelName(*FPackageName::GetShortName(LevelPath.GetLongPackageName()));
		if (RouteLevelName == LevelName)
		{
			return Route;
		}
	}

	return nullptr;
}

FName UFTGameFlowSubsystem::ResolveLoadingLevelName() const
{
	if (const UFTGameDataAsset* GameData = UFTAssetManager::Get().GetGameData())
	{
		if (!GameData->LoadingLevel.IsNull())
		{
			return FName(*FPackageName::GetShortName(GameData->LoadingLevel.ToSoftObjectPath().GetLongPackageName()));
		}
	}

	UE_LOG(LogFTFlow, Warning, TEXT("Loading level is not set in FTGameDataAsset."));
	return NAME_None;
}

FName UFTGameFlowSubsystem::ResolveCurrentWorldLevelName() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return NAME_None;
	}

	FString MapName = FPackageName::GetShortName(World->GetMapName());
	if (MapName.StartsWith(TEXT("UEDPIE_")))
	{
		const int32 PiePrefixIndex = MapName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, FString(TEXT("UEDPIE_")).Len());
		if (PiePrefixIndex != INDEX_NONE)
		{
			MapName = MapName.Mid(PiePrefixIndex + 1);
		}
	}

	return MapName.IsEmpty() ? NAME_None : FName(*MapName);
}

FName UFTGameFlowSubsystem::ResolveLevelNameForState(EFTFlowStateType State) const
{
	if ((State == EFTFlowStateType::RaidEntering
		|| State == EFTFlowStateType::RaidInProgress
		|| State == EFTFlowStateType::Escaping
		|| State == EFTFlowStateType::Escaped
		|| State == EFTFlowStateType::Failed)
		&& !PendingRaidLevelName.IsNone())
	{
		return PendingRaidLevelName;
	}

	if (const FFTFlowLevelRouteStruct* Route = FindFlowLevelRouteByState(State))
	{
		if (!Route->Level.IsNull())
		{
			return FName(*FPackageName::GetShortName(Route->Level.ToSoftObjectPath().GetLongPackageName()));
		}
	}

	UE_LOG(LogFTFlow, Warning, TEXT("No level route is set for flow state. State=%d"),
		static_cast<uint8>(State));
	return NAME_None;
}

EFTFlowStateType UFTGameFlowSubsystem::ResolveFlowStateForCurrentWorld() const
{
	const FName CurrentLevelName = ResolveCurrentWorldLevelName();
	if (CurrentLevelName.IsNone() || CurrentLevelName == ResolveLoadingLevelName())
	{
		return CurrentFlowState;
	}

	if (const FFTFlowLevelRouteStruct* Route = FindFlowLevelRouteByLevelName(CurrentLevelName))
	{
		if (Route->State == EFTFlowStateType::RaidEntering
			|| Route->State == EFTFlowStateType::Escaping
			|| Route->State == EFTFlowStateType::Escaped
			|| Route->State == EFTFlowStateType::Failed)
		{
			return EFTFlowStateType::RaidInProgress;
		}

		return Route->State;
	}

	return CurrentFlowState;
}

bool UFTGameFlowSubsystem::ShouldUseLoadingForState(EFTFlowStateType State, FName RequestedLevelName) const
{
	if (!RequestedLevelName.IsNone())
	{
		if (const FFTFlowLevelRouteStruct* Route = FindFlowLevelRouteByLevelName(RequestedLevelName))
		{
			return Route->bUseLoadingLevel;
		}

		if (State == EFTFlowStateType::RaidEntering
			|| State == EFTFlowStateType::RaidInProgress
			|| State == EFTFlowStateType::Escaping)
		{
			UE_LOG(LogFTFlow, Warning, TEXT("No flow route found for requested raid level. Falling back to loading level. State=%d RequestedLevel=%s"),
				static_cast<uint8>(State),
				*RequestedLevelName.ToString());
			return true;
		}
	}

	if (const FFTFlowLevelRouteStruct* Route = FindFlowLevelRouteByState(State))
	{
		return Route->bUseLoadingLevel;
	}

	return false;
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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UFTSaveSubsystem>())
		{
			SaveSubsystem->SaveBeforeLevelTransition(LevelName, CurrentFlowState);
		}
	}

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
			UIManager->HidePauseMenu();
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
	PreloadCurrentFlowStateForTest();
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

void UFTGameFlowSubsystem::PreloadCurrentFlowStateForTest() const
{
	// Test Code: Loading Level을 거치지 않고 현재 FlowState 변경만으로 preload 동작을 확인하기 위한 임시 경로다.
	// 정규 흐름에서는 AFTLoadingGameMode가 FlowState에 맞는 LevelPreloadDataAsset을 선택해 로드한다.
	PreloadCurrentStateAssetsAsync(
		FSimpleDelegate::CreateLambda([]()
		{
			UE_LOG(LogFTFlow, Log, TEXT("Test preload for current flow state completed."));
		}),
		FFTAssetLoadProgressDelegate::CreateLambda([](const FString& AssetName, int32 CompletedCount, int32 TotalCount)
		{
			UE_LOG(LogFTFlow, Verbose, TEXT("Test preload progress: %s (%d / %d)"), *AssetName, CompletedCount, TotalCount);
		}));
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
