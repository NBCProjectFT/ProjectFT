#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "ProjectFT/Enum/FTFlowStateType.h"
#include "ProjectFT/Enum/FTPauseMenuConfirmType.h"
#include "FTPauseMenuViewModel.generated.h"

class USoundClass;
class USoundMix;
class UWorld;

UCLASS(BlueprintType)
class PROJECTFT_API UFTPauseMenuViewModel : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void Initialize(UObject* InWorldContextObject);

	UFUNCTION(BlueprintPure, Category = "FT|Pause")
	EFTFlowStateType GetCurrentFlowState() const;

	UFUNCTION(BlueprintPure, Category = "FT|Pause")
	bool IsCurrentFlowStateBase() const;

	UFUNCTION(BlueprintPure, Category = "FT|Pause")
	bool ShouldShowReturnToBaseButton() const;

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void ApplyMasterVolume(USoundMix* MasterSoundMix, USoundClass* MasterSoundClass, float Volume) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void ExecuteConfirmedAction(EFTPauseMenuConfirmType ConfirmType) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void BroadcastFlowRequest(const FGameplayTag& RequestTag) const;

	UFUNCTION(BlueprintCallable, Category = "FT|Pause")
	void ClearPlayerInventoryForReturnToBase() const;

private:
	UWorld* ResolveWorld() const;
	UGameInstance* ResolveGameInstance() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> WorldContextObject;
};
