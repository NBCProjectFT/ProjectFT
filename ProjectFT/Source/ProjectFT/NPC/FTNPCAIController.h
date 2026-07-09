
#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/AI/FTAIControllerBase.h"
#include "FTNPCAIController.generated.h"

class UAIPerceptionComponent;
class UStateTreeAIComponent;
class UAISenseConfig_Sight;
class UFTNPCReportComponent;
struct FFTMessagePayloadStruct;
UCLASS()
class PROJECTFT_API AFTNPCAIController : public AFTAIControllerBase
{
	GENERATED_BODY()

public:
	AFTNPCAIController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	TObjectPtr<UAIPerceptionComponent> NPCPerceptionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	TObjectPtr<UStateTreeAIComponent> NPCStateTreeAIComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	TObjectPtr<UFTNPCReportComponent> NPCReportComponent;

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
	bool bIsTargetActivelyStealing = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Target")
	bool bCanStartReportFlow = false;

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
	float ObservedStealingMemorySeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Debug")
	bool bLogShoppingDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Debug")
	bool bLogReportDebug = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	float CurrentReportProgress = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bReportCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bReportCancelled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bIsStunned = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bObservedShelfDamaged = false;

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Report")
	void EnterSuspicious();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Report")
	void EnterReporting();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Report")
	bool TickReporting(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Report")
	void CancelReport();

	void HandleStunStateChanged(bool bStunned);

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Target")
	void UpdateTargetState();

	UFUNCTION(BlueprintPure, Category = "FT|NPC|Target")
	bool CanStartReportFlow() const;


private:
	float LastObservedStealingTime = -FLT_MAX;
	bool bLastLoggedHasSeenTarget = false;
	bool bLastLoggedIsTargetStealing = false;
	bool bLastLoggedCanStartReportFlow = false;
	
	FGameplayMessageListenerHandle ShelfDamagedListenerHandle;
	void OnShelfDamaged(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	AActor* ResolvePlayerActor(AActor* DamageCauser) const;
	bool IsPlayerActor(const AActor* Actor) const;
	bool IsTargetCurrentlyVisible() const;
	bool IsTargetStealing(const AActor* Actor) const;
	void SyncReportStateFromComponent();
	void DrawSightDebug() const;
	void LogReportConditionDebug(bool bTargetCurrentlyStealing);
};
