#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/AI/FTAIControllerBase.h"
#include "FTSecurityAIController.generated.h"

class UAIPerceptionComponent;
class UStateTreeAIComponent;
class UAISenseConfig_Sight;
class UFTSecurityCallComponent;
class UFTSecurityCaptureStateComponent;
class UFTSecurityPursuitStateComponent;
class UFTSecurityResponseComponent;
class UFTSecurityReturnComponent;
class UFTSecurityTargetComponent;
class AActor;
struct FFTNPCReportPayloadStruct;
struct FFTMessagePayloadStruct;
struct FFTSecurityChaseGaugePayloadStruct;
struct FFTSecurityResponsePayloadStruct;
struct FFTCharacterAttackedPayloadStruct;
UCLASS()
class PROJECTFT_API AFTSecurityAIController : public AFTAIControllerBase
{
	GENERATED_BODY()
	friend class UFTSecurityResponseComponent;
	friend class UFTSecurityReturnComponent;
	friend class UFTSecurityCaptureStateComponent;
	friend class UFTSecurityPursuitStateComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Call")
	TObjectPtr<UFTSecurityCallComponent> SecurityCallComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Target")
	TObjectPtr<UFTSecurityTargetComponent> SecurityTargetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Response")
	TObjectPtr<UFTSecurityResponseComponent> SecurityResponseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Return")
	TObjectPtr<UFTSecurityReturnComponent> SecurityReturnComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	TObjectPtr<UFTSecurityCaptureStateComponent> SecurityCaptureStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Pursuit")
	TObjectPtr<UFTSecurityPursuitStateComponent> SecurityPursuitStateComponent;
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
public:
	void SetTargetActor(AActor* NewTargetActor);
	void StartChase();
	void ReadyDespawn();
	void HandleControlledPawnDeath();

	/** KnockedOut 상태 연출이 끝난 뒤 Pawn을 제거한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Security|Death")
	void FinishKnockedOut();
	
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

	/** 짧은 가림이나 이동 회전으로 시야가 끊겨도 추격 상태를 유지하는 시간이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Perception", meta = (ClampMin = "0.0"))
	float TargetSightLostGracePeriod = 0.75f;

	/** 전방 시야 밖이라도 가까운 대상은 감지할 수 있는 근접 원형 감지 반경이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Perception", meta = (ClampMin = "0.0"))
	float CloseDetectionRadius = 300.0f;

	/** 전방 시야가 아니라 근접 원형 범위로 대상을 감지했는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Perception")
	bool bDetectedTargetByCloseRange = false;

	/** 현재 타겟이 범죄행위와 연결된 대상으로 확인되었는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Perception")
	bool bHasObservedCrime = false;
	
	UFUNCTION(BlueprintPure, Category = "FT|Security")
	AActor* GetTargetActor() const;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	FVector HomeLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	FRotator HomeRotation = FRotator::ZeroRotator;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	FVector ReturnLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	bool bSpawnedFromSecurityRoom = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security")
	bool bReturning = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Return", meta = (ClampMin = "0.0"))
	float ReturnCollisionIgnoreDistance = 250.0f;

	/** 복귀 이동 성공을 실제 복귀 완료로 인정할 최대 거리다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Return", meta = (ClampMin = "0.0"))
	float ReturnCompletionDistance = 250.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|ChaseGauge")
	float SecurityChaseGauge = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|ChaseGauge")
	bool bSecurityChaseActive = false;

	/** 현재 추격 대상이 보안요원에게 붙잡힌 상태인지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	bool bTargetCaptured = false;

	/** 이 AI가 현재 붙잡힌 대상의 Captor인지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	bool bIsCaptor = false;

	/** 다른 보안요원이 현재 대상을 붙잡았는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	bool bIsTargetCapturedByOtherSecurity = false;

	/** StateTree가 Return 상태로 전환해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	bool bReturnRequested = false;

	/** StateTree가 탈출 위치를 조사해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	bool bInvestigateRequested = false;

	/** 이 AI가 탈출 직후 Stun 상태로 전환해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	bool bStunRequested = false;

	/** 보안요원의 Grab 어빌리티가 현재 실행 중인지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	bool bIsGrabbing = false;

	/** 보안요원의 ASC가 실제 Stun 태그를 보유하고 있는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Capture")
	bool bIsStunned = false;

	/** HP가 0이 되어 무력화 상태로 진입했는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|State")
	bool bKnockedOut = false;

	/** 플레이어를 붙잡기 위해 Approach 상태로 진입할 보안요원인지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination")
	bool bIsAttackLeader = false;

	/** 플레이어를 붙잡는 Approach 리더 후보가 될 수 있는지 나타낸다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Security|Coordination")
	bool bCanBeCaptureLeader = true;

	/** 현재 보안요원이 추격 상태에 참여하고 있는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FT|Security|Coordination")
	bool bParticipatingInChase = false;

	/** Coordination Component가 이 보안요원에게 배정한 포위 이동 위치다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination")
	FVector EncircleSlotLocation = FVector::ZeroVector;

	/** StateTree가 EncircleSlotLocation을 이동 목표로 사용할 수 있는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination")
	bool bHasEncircleSlot = false;

	/** Chase에서 Encircle로 전환할 최대 타겟 거리다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination", meta = (ClampMin = "0.0"))
	float EncircleEnterDistance = 900.0f;

	/** Encircle에서 Chase로 전환할 타겟 거리다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination", meta = (ClampMin = "0.0"))
	float EncircleExitDistance = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Debug")
	bool bDrawAttackRangeDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Debug")
	bool bLogSecurityEventDebug = false;

private:
	FGameplayMessageListenerHandle SecurityCalledListenerHandle;
	FGameplayMessageListenerHandle ChaseGaugeChangedListenerHandle;
	FGameplayMessageListenerHandle ChaseEndedListenerHandle;
	FGameplayMessageListenerHandle SecurityDeployedListenerHandle;
	FGameplayMessageListenerHandle SecurityTargetCapturedListenerHandle;
	FGameplayMessageListenerHandle SecurityTargetEscapedListenerHandle;
	FGameplayMessageListenerHandle ShelfDamagedListenerHandle;
	FGameplayMessageListenerHandle CharacterAttackedListenerHandle;
	void OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnShelfDamaged(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void OnCharacterAttacked(FGameplayTag Channel, const FFTCharacterAttackedPayloadStruct& Payload);
	void OnSecurityTargetCaptured(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnSecurityTargetEscaped(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);
	void OnChaseGaugeChanged(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void OnChaseEnded(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);
	void OnSecurityDeployed(FGameplayTag Channel, const FFTSecurityResponsePayloadStruct& Payload);
	void UpdateTargetState();
	void UpdateTargetFocus();
	void UpdateAbilityState();
	void UpdateChaseGaugeTargetSeenState();
	void UpdateSecurityCallGauge(float DeltaTime);
	bool bReturnFailureLogged = false;
	bool bReturnCollisionIgnored = false;
	bool bCanRequestSecuritySupport = false;

	UPROPERTY()
	TObjectPtr<AActor> SecurityRoomActor;
	bool IsPlayerActor(const AActor* Actor) const;
	AActor* ResolvePlayerActor(AActor* DamageCauser) const;
	bool IsTargetStealing(const AActor* Actor) const;
	bool IsTargetCurrentlyVisible() const;
	bool IsActorDetectedByCloseRange(AActor* Actor) const;
	bool IsTargetDetectedByCloseRange() const;
	void DrawSightDebug() const;
};
