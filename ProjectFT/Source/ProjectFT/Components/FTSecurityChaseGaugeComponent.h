#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Core/GameplayMessageProcessor.h"
#include "FTSecurityChaseGaugeComponent.generated.h"

struct FFTNPCReportPayloadStruct;
struct FFTSecurityChaseGaugePayloadStruct;

/**
 * 보안요원의 추격 게이지를 관리하는 컴포넌트이다.
 * 
 * 보안 호출, 보안요원 시야 감지/놓침 메시지를 수신하고,
 * 추격 상태와 추격 게이지를 갱신한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityChaseGaugeComponent : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	UFTSecurityChaseGaugeComponent();

	/** 추격 게이지 감소 처리를 위해 매 프레임 호출된다. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 현재 추격 게이지 값을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|ChaseGauge")
	float GetChaseGauge() const;

	/** 현재 추격 게이지 비율을 0~1 범위로 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|ChaseGauge")
	float GetChaseGaugeRatio() const;

	/** 현재 추격 상태가 활성화되어 있는지를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|ChaseGauge")
	bool IsChaseActive() const;

	/** 하나 이상의 보안요원이 TargetActor를 보고 있는지 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|ChaseGauge")
	bool IsAnySecuritySeeingTarget() const;

	/** 추격 게이지와 추격 상태를 초기화한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Security|ChaseGauge")
	void ResetChaseGauge();

protected:
	/** 추격 상태를 변경하는 Gameplay Message 리스너를 등록한다. */
	virtual void StartListening() override;

	/** 추격 게이지 최대값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|ChaseGauge", meta = (ClampMin = "1.0"))
	float MaxChaseGauge = 100.0f;

	/** 보안요원이 대상을 보지 못할 때 초당 감소하는 추격 게이지 값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|ChaseGauge", meta = (ClampMin = "0.0"))
	float ChaseGaugeDecayPerSecond = 10.0f;

private:
	/** 현재 추격 게이지 값이다. */
	UPROPERTY(VisibleInstanceOnly, Category = "FT|Security|ChaseGauge")
	float CurrentChaseGauge = 0.0f;

	/** 추격 활성화 여부다. */
	UPROPERTY(VisibleInstanceOnly, Category = "FT|Security|ChaseGauge")
	bool bChaseActive = false;

	/** 현재 추격 대상 액터다. */
	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	/** 보안요원이 마지막으로 확인한 타겟 위치다. */
	FVector LastKnownLocation = FVector::ZeroVector;
	
	/** 현재 대상을 보고 있는 보안요원 목록이다. */
	TSet<TWeakObjectPtr<AActor>> SecuritiesSeeingTarget;

	/** 보안 호출을 수신해 추격을 활성화하고 게이지를 최대치로 설정한다. */
	void OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload);

	/** 대상을 감지한 보안요원을 등록하고 추격 게이지를 최대치로 복원한다. */
	void OnSecurityTargetSeen(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);

	/** 대상을 놓친 보안요원을 감지 목록에서 제거한다. */
	void OnSecurityTargetLost(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload);

	/** 추격 게이지를 유효 범위로 설정하고 변경 및 종료 메시지를 발행한다. */
	void SetChaseGauge(float NewChaseGauge);

	/** 현재 추격 게이지 상태를 메시지로 발행한다. */
	void BroadcastGaugeChanged();

	/** 추격 종료 메시지를 발행한다. */
	void BroadcastChaseEnded();

	/** 파괴된 보안요원 참조를 감지 목록에서 제거한다. */
	void RemoveInvalidSecurityActors();
};
