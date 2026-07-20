#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "FTInstantReportComponent.generated.h"

class AFTAIControllerBase;
class UAISenseConfig_Sight;
struct FFTCharacterAttackedPayloadStruct;

/**
 * 캐셔/직원처럼 신고 게이지 없이 즉시 신고하는 AI가 사용하는 감지/신고 컴포넌트다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTInstantReportComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTInstantReportComponent();

	void Initialize(AFTAIControllerBase* InOwnerController, UAISenseConfig_Sight* InSightConfig);
	void StartListening();
	void StopListening();
	void TickInstantReport();
	void HandleTargetPerceptionUpdated(AActor* Actor);

	/** StateTree Report 상태에서 호출해 대기 중인 즉시 신고 메시지를 발행한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|InstantReport")
	bool BroadcastRequestedReport();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|InstantReport|Target")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|InstantReport|Target")
	bool bHasSeenTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|InstantReport|Report")
	bool bHasReported = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|InstantReport|Report")
	bool bReportRequested = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|InstantReport|Report")
	bool bReportOnlyOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|InstantReport|Debug")
	bool bLogInstantReportDebug = false;

private:
	UPROPERTY()
	TObjectPtr<AFTAIControllerBase> OwnerController;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	FGameplayMessageListenerHandle CharacterAttackedListenerHandle;
	FVector PendingReportLocation = FVector::ZeroVector;

	void OnCharacterAttacked(FGameplayTag Channel, const FFTCharacterAttackedPayloadStruct& Payload);
	void UpdateTargetState();
	void TryReportObservedStealing();
	void RequestInstantReport(AActor* SuspectActor, const FVector& ReportLocation);
	AActor* ResolvePlayerActor(AActor* DamageCauser) const;
	bool IsPlayerActor(const AActor* Actor) const;
	bool IsTargetStealing(const AActor* Actor) const;
	bool CanWitnessActor(AActor* Actor) const;
	bool IsActorVisibleBySight(AActor* Actor) const;
};
