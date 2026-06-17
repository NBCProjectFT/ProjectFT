#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Enum/FTFlowStateType.h"
#include "FTGameFlowSubsystem.generated.h"

UCLASS()
class PROJECTFT_API UFTGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "FT|Flow")
	EFTFlowStateType CurrentFlowState = EFTFlowStateType::Base;

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestStartRaid();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestEscapeRaid();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void RequestFailRaid();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void EnterSettlement();

	UFUNCTION(BlueprintCallable, Category = "FT|Flow")
	void ReturnToBase();

private:
	void SetFlowState(EFTFlowStateType NewFlowState);
	void BroadcastFlowEvent(FGameplayTag Channel) const;
};
