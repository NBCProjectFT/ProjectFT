#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "FTSecurityAIController.generated.h"

class UAIPerceptionComponent;
class UStateTreeAIComponent;
class UAISenseConfig_Sight;
class AActor;
struct FFTNPCReportPayloadStruct;
struct FFTSecurityChaseGaugePayloadStruct;
struct FFTSecurityResponsePayloadStruct;
UCLASS()
class PROJECTFT_API AFTSecurityAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFTSecurityAIController();

protected:
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	TObjectPtr<UStateTreeAIComponent> SecurityStateTreeAIComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	TObjectPtr<UAIPerceptionComponent> SecurityPerceptionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
public:
	void SetTargetActor(AActor* NewTargetActor);
	void StartChase();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FT|Security")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	FVector InvestigateLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	bool bSecurityCalled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	bool bHasSeenTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	bool bIsTargetInAttackRange = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	float TargetDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security")
	float AttackRange = 150.0f;
	
	UFUNCTION(BlueprintPure, Category = "FT|Security")
	AActor* GetTargetActor() const;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	FVector HomeLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	FVector ReturnLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	bool bSpawnedFromSecurityRoom = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	bool bReturning = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Return", meta = (ClampMin = "0.0"))
	float ReturnCollisionIgnoreDistance = 250.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|ChaseGauge")
	float SecurityChaseGauge = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|ChaseGauge")
	bool bSecurityChaseActive = false;

private:
	FGameplayMessageListenerHandle SecurityCalledListenerHandle;
	FGameplayMessageListenerHandle ChaseGaugeChangedListenerHandle;
	FGameplayMessageListenerHandle ChaseEndedListenerHandle;
	FGameplayMessageListenerHandle SecurityDeployedListenerHandle;
	void OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnChaseGaugeChanged(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void OnChaseEnded(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void OnSecurityDeployed(FGameplayTag Channel, const FFTSecurityResponsePayloadStruct& Payload);
	void UpdateTargetState();
	void UpdateChaseGaugeTargetSeenState();
	void UpdateReturnCollision();
	void CompleteReturn();
	bool bReportedTargetSeenToChaseGauge = false;
	bool bReturnFailureLogged = false;
	bool bReturnCollisionIgnored = false;

	UPROPERTY()
	TObjectPtr<AActor> SecurityRoomActor;
	bool IsPlayerActor(const AActor* Actor) const;
	bool IsTargetStealing(const AActor* Actor) const;
	bool IsTargetCurrentlyVisible() const;
	void DrawSightDebug() const;
};
