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
UCLASS()
class PROJECTFT_API AFTSecurityAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFTSecurityAIController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
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
	
private:
	FGameplayMessageListenerHandle SecurityCalledListenerHandle;
	void OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void UpdateTargetState();
	bool IsTargetCurrentlyVisible() const;
	void DrawSightDebug() const;
};
