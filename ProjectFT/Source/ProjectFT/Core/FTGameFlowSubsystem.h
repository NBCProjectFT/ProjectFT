#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "FTGameFlowSubsystem.generated.h"

struct FFTFlowLevelRouteStruct;
struct FFTMessagePayloadStruct;
class UFTLevelPreloadDataAsset;

UCLASS()
class PROJECTFT_API UFTGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Flow")
	EFTFlowStateType CurrentFlowState = EFTFlowStateType::MainMenu;

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	EFTFlowStateType GetCurrentFlowState() const { return CurrentFlowState; }

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestStartGame();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestContinueGame();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestStartRaid();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	bool RequestStartRaidAtLevel(FName TargetLevelName);

	UFUNCTION(BlueprintPure, Category = "FT|Flow")
	bool CanStartRaidAtLevel(FName TargetLevelName) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestEscapeRaid();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestCancelEscape();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestCompleteEscape();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestFailRaid();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void ReturnToBase();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void ReturnToMainMenu();

	// Called by LoadingGameMode after its preload is done and the player confirms.
	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void CompleteLoadingAndOpenCurrentStateLevel();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void SyncFlowStateWithCurrentLevel();

	void PreloadCurrentStateAssetsAsync(
		FSimpleDelegate OnLoaded,
		FFTAssetLoadProgressDelegate OnProgress = FFTAssetLoadProgressDelegate()) const;

	TSoftObjectPtr<UFTLevelPreloadDataAsset> GetLevelPreloadDataAssetForState(EFTFlowStateType State) const;
	TSoftObjectPtr<UFTLevelPreloadDataAsset> GetCurrentStateLevelPreloadDataAsset() const;
	TSoftObjectPtr<UFTLevelPreloadDataAsset> ResolveLevelPreloadDataAssetForCurrentState() const;

private:
	void HandleStartGameMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void HandleFlowRequestMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void TravelToState(EFTFlowStateType TargetFlowState, FName RequestedLevelName = NAME_None);
	void TravelToStateWithLoading(EFTFlowStateType TargetFlowState);
	const FFTFlowLevelRouteStruct* FindFlowLevelRouteByState(EFTFlowStateType State) const;
	const FFTFlowLevelRouteStruct* FindFlowLevelRouteByLevelName(FName LevelName) const;
	FName ResolveLoadingLevelName() const;
	FName ResolveCurrentWorldLevelName() const;
	FName ResolveLevelNameForState(EFTFlowStateType State) const;
	EFTFlowStateType ResolveFlowStateForCurrentWorld() const;
	bool ShouldUseLoadingForState(EFTFlowStateType State, FName RequestedLevelName = NAME_None) const;
	void ApplyBGMForLevel(FName LevelName) const;
	void OpenLevelByName(FName LevelName) const;
	void RestoreMenuInputBeforeTravel(FName LevelName) const;
	void SetFlowState(EFTFlowStateType NewFlowState);
	void HandleFlowStateEntered(EFTFlowStateType NewFlowState);
	void PreloadCurrentFlowStateForTest() const;
	void BroadcastFlowStateChanged() const;
	void BroadcastFlowEvent(FGameplayTag Channel) const;

	TArray<FGameplayMessageListenerHandle> FlowRequestListenerHandles;

	UPROPERTY(Transient)
	FName PendingRaidLevelName = NAME_None;
};
