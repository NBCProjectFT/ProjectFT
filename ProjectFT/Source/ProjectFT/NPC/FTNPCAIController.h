
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "FTNPCAIController.generated.h"

class UAIPerceptionComponent;
class UStateTreeAIComponent;
class UAISenseConfig_Sight;
UCLASS()
class PROJECTFT_API AFTNPCAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFTNPCAIController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	TObjectPtr<UAIPerceptionComponent> NPCPerceptionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	TObjectPtr<UStateTreeAIComponent> NPCStateTreeAIComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Target")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Target")
	bool bHasSeenTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Target")
	bool bIsTargetStealing = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Target")
	float TargetDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	FVector ShoppingTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	bool bHasShoppingTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	FName ShoppingPointTag = TEXT("CustomerShoppingPoint");

	UFUNCTION(BlueprintCallable, Category = "FT|NPC")
	bool PickRandomShoppingTarget();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Wander")
	bool PickRandomWanderTarget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ReportCancelDistance = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float ObservedStealingMemorySeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Debug")
	bool bDrawSightDebug = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float CurrentReportProgress = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bReportCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bReportCancelled = false;

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Report")
	void EnterSuspicious();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Report")
	void EnterReporting();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Report")
	bool TickReporting(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Report")
	void CancelReport();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Target")
	void UpdateTargetState();

	UFUNCTION(BlueprintPure, Category = "FT|NPC|Target")
	bool CanStartReportFlow() const;

private:
	float ReportElapsedTime = 0.0f;
	int32 LastLoggedReportPercent = -1;
	float LastObservedStealingTime = -FLT_MAX;
	bool bLastLoggedHasSeenTarget = false;
	bool bLastLoggedIsTargetStealing = false;
	bool bLastLoggedCanStartReportFlow = false;

	bool IsPlayerActor(const AActor* Actor) const;
	bool IsTargetCurrentlyVisible() const;
	bool IsTargetStealing(const AActor* Actor) const;
	bool ShouldCancelReport() const;
	void CompleteReport();
	void DrawSightDebug() const;
	void LogReportConditionDebug(bool bTargetCurrentlyStealing);
};
