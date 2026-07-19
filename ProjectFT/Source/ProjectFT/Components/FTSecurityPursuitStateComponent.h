#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FTSecurityPursuitStateComponent.generated.h"

class AFTSecurityAIController;
struct FFTSecurityChaseGaugePayloadStruct;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityPursuitStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityPursuitStateComponent();

	/** 이 보안요원이 타겟을 보고 있는지 추격 게이지 시스템에 알린다. */
	void UpdateTargetSeenState(AFTSecurityAIController* Controller);

	/** 전체 추격 게이지 변경 메시지를 받아 이 보안요원의 StateTree용 상태값을 갱신한다. */
	void HandleChaseGaugeChanged(AFTSecurityAIController* Controller, const FFTSecurityChaseGaugePayloadStruct& Payload) const;

	/** 추격 게이지 종료 메시지를 받아 복귀 상태로 전환할 준비를 한다. */
	void HandleChaseEnded(AFTSecurityAIController* Controller, const FFTSecurityChaseGaugePayloadStruct& Payload);

	/** 컨트롤러 종료 시 TargetSeen 상태를 정리한다. */
	void ClearReportedTargetSeen(AFTSecurityAIController* Controller);

private:
	bool bReportedTargetSeenToChaseGauge = false;
};
