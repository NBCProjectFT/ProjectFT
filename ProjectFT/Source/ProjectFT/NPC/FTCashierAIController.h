#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/AI/FTAIControllerBase.h"
#include "FTCashierAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UStateTreeAIComponent;
struct FFTCharacterAttackedPayloadStruct;

UCLASS()
class PROJECTFT_API AFTCashierAIController : public AFTAIControllerBase
{
	GENERATED_BODY()

public:
	AFTCashierAIController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier")
	TObjectPtr<UStateTreeAIComponent> CashierStateTreeAIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier")
	TObjectPtr<UAIPerceptionComponent> CashierPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Target")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Target")
	bool bHasSeenTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Report")
	bool bHasReported = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Report")
	bool bReportOnlyOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Debug")
	bool bLogCashierDebug = false;

private:
	FGameplayMessageListenerHandle CharacterAttackedListenerHandle;

	void OnCharacterAttacked(FGameplayTag Channel, const FFTCharacterAttackedPayloadStruct& Payload);
	void UpdateTargetState();
	void TryReportObservedStealing();
	void BroadcastInstantReport(AActor* SuspectActor, const FVector& ReportLocation);
	AActor* ResolvePlayerActor(AActor* DamageCauser) const;
	bool IsPlayerActor(const AActor* Actor) const;
	bool IsTargetStealing(const AActor* Actor) const;
	bool CanWitnessActor(AActor* Actor) const;
};
