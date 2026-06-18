#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "FTSecurityAIController.generated.h"

class UAIPerceptionComponent;
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
	TObjectPtr<UAIPerceptionComponent> SecurityPerceptionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	TObjectPtr<AActor> TargetActor;
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
public:
	void SetTargetActor(AActor* NewTargetActor);
	void StartChase();
	
private:
	FGameplayMessageListenerHandle SecurityCalledListenerHandle;
	void OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void DrawSightDebug() const;
};
