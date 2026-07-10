#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FTNPCReportComponent.generated.h"

class AFTNPCAIController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTNPCReportComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTNPCReportComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportDecayDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportCancelDistance = 1800.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float CurrentReportProgress = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bReportCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bReportCancelled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bObservedShelfDamaged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bObservedAssault = false;

	void EnterReporting();
	bool TickReporting(float DeltaTime);
	void CancelReport();
	void HandleReportFlowAvailability(bool bCanStartReportFlow, bool bLogReportDebug);
	void MarkObservedShelfDamage();
	void MarkObservedAssault();
	void HandleStunStateChanged(bool bStunned);

private:
	float ReportElapsedTime = 0.0f;
	int32 LastLoggedReportPercent = -1;
	int32 LastLoggedReportDecayPercent = 101;

	AFTNPCAIController* GetNPCAIController() const;
	bool ShouldCancelReport(const AFTNPCAIController* Controller) const;
	void CompleteReport();
	void BroadcastReportMessage(FGameplayTag Channel, AActor* TargetActor, float InReportAmount, float ReportProgress) const;
	void ResetReportState();
};
