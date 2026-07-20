#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "ProjectFT/AI/FTAIControllerBase.h"
#include "FTCashierAIController.generated.h"

class UFTInstantReportComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UStateTreeAIComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Report")
	TObjectPtr<UFTInstantReportComponent> InstantReportComponent;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Target")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Target")
	bool bHasSeenTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Report")
	bool bHasReported = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Report")
	bool bReportRequested = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Report")
	bool bReportOnlyOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Cashier|Debug")
	bool bLogCashierDebug = false;

	/** StateTree Report 상태에서 호출해 대기 중인 신고 메시지를 발행한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Cashier|Report")
	bool BroadcastRequestedReport();

private:
	void SyncInstantReportStateFromComponent();
	bool IsPlayerActor(const AActor* Actor) const;
};
