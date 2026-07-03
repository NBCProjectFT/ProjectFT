#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "FTGameFlowSubsystem.generated.h"

struct FFTFlowStateDefinition;
struct FFTMessagePayloadStruct;

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
	void RequestStartRaid();

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

	// Called by LoadingGameMode after its preload is done and the player confirms.
	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void CompleteLoadingAndOpenCurrentStateLevel();

	void PreloadCurrentStateAssetsAsync(
		FSimpleDelegate OnLoaded,
		FFTAssetLoadProgressDelegate OnProgress = FFTAssetLoadProgressDelegate()) const;

private:
	void HandleStartGameMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void HandleFlowRequestMessage(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void TravelToState(EFTFlowStateType TargetFlowState);
	void TravelToStateWithLoading(EFTFlowStateType TargetFlowState);
	const FFTFlowStateDefinition* FindFlowStateDefinition(EFTFlowStateType State) const;
	FName ResolveLoadingLevelName() const;
	FName ResolveLevelNameForState(EFTFlowStateType State) const;
	bool ShouldUseLoadingForState(EFTFlowStateType State) const;
	void OpenLevelByName(FName LevelName) const;
	void RestoreMenuInputBeforeTravel(FName LevelName) const;
	void SetFlowState(EFTFlowStateType NewFlowState);
	void HandleFlowStateEntered(EFTFlowStateType NewFlowState);
	void BroadcastFlowStateChanged() const;
	void BroadcastFlowEvent(FGameplayTag Channel) const;

	TArray<FGameplayMessageListenerHandle> FlowRequestListenerHandles;
};
