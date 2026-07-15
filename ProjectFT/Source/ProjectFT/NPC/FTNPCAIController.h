
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
class UFTNPCReactionComponent;
class UFTNPCShoppingComponent;
struct FFTMessagePayloadStruct;
struct FFTCharacterDamagePayloadStruct;
struct FFTCharacterAttackedPayloadStruct;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction")
	TObjectPtr<UFTNPCReactionComponent> NPCReactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Shopping")
	TObjectPtr<UFTNPCShoppingComponent> NPCShoppingComponent;

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
	FVector ShoppingLookLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	float ShoppingTargetAcceptanceRadius = 100.0f;

	/** 쇼핑 중 시선 목표가 바뀔 때 한 번에 꺾이지 않도록 보간하는 속도다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Shopping", meta = (ClampMin = "0.0"))
	float ShoppingLookInterpSpeed = 4.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC")
	bool bHasShoppingTarget = false;

	UFUNCTION(BlueprintCallable, Category = "FT|NPC")
	bool PickRandomShoppingTarget();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Wander")
	bool PickRandomWanderTarget();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Wander")
	void ReleaseShoppingTarget();

	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Wander")
	void StartShoppingLook();

	/** 플레이어의 반대 방향으로 도망갈 NavMesh 위치를 계산한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Reaction")
	bool PickFleeLocationFrom(AActor* ThreatActor);

	/**
	 * 현재 TargetActor를 기준으로 도망을 요청한다.
	 *
	 * @return 도망 위치를 찾고 도망 요청을 설정했으면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Reaction")
	bool RequestFleeFromTarget();

	/** 도망 상태가 끝난 뒤 도망 관련 요청 값을 초기화한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Reaction")
	void FinishFlee();

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
	
	/** 손님NPC가 매대 공격을 목격했는지 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bObservedShelfDamaged = false;

	/** 손님NPC가 플레이어의 공격을 목격했는지 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Report")
	bool bObservedAssault = false;

	/** 스턴이 풀린 뒤 공포 상태로 진입해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction")
	bool bPanicRequested = false;

	/** 플레이어에게서 도망 상태로 진입해야 하는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction")
	bool bFleeRequested = false;

	/** HP가 0이 되어 무력화 상태로 진입했는지 나타낸다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction")
	bool bKnockedOut = false;

	/** 도망 상태에서 이동할 목표 위치다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction")
	FVector FleeLocation = FVector::ZeroVector;

	/** 도망 상태에서 사용할 이동 속도다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|NPC|Reaction", meta = (ClampMin = "0.0"))
	float FleeMoveSpeed = 600.0f;

	/** 공포 상태에 진입할 때 위협 대상을 바라보도록 설정한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|NPC|Reaction")
	void EnterPanic();

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

	void ClearReactionFocusState();

	bool IsUsingReportFocus() const;

private:
	float LastObservedStealingTime = -FLT_MAX;
	bool bLastLoggedHasSeenTarget = false;
	bool bLastLoggedIsTargetStealing = false;
	bool bLastLoggedCanStartReportFlow = false;

	bool bUsingReportFocus = false;
	
	FGameplayMessageListenerHandle ShelfDamagedListenerHandle;
	FGameplayMessageListenerHandle CharacterDamagedListenerHandle;
	FGameplayMessageListenerHandle CharacterAttackedListenerHandle;
	void OnShelfDamaged(FGameplayTag Channel, const FFTMessagePayloadStruct& Payload);
	void OnCharacterDamaged(FGameplayTag Channel, const FFTCharacterDamagePayloadStruct& Payload);
	void OnCharacterAttacked(FGameplayTag Channel, const FFTCharacterAttackedPayloadStruct& Payload);
	AActor* ResolvePlayerActor(AActor* DamageCauser) const;
	bool IsPlayerActor(const AActor* Actor) const;
	bool IsTargetCurrentlyVisible() const;
	bool IsTargetStealing(const AActor* Actor) const;
	void SyncReportStateFromComponent();
	void UpdateReportFocus();
	void DrawSightDebug() const;
	void LogReportConditionDebug(bool bTargetCurrentlyStealing);
};
