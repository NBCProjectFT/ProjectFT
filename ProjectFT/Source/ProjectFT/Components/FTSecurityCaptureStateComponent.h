#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FTSecurityCaptureStateComponent.generated.h"

class AFTSecurityAIController;
struct FFTNPCReportPayloadStruct;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityCaptureStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityCaptureStateComponent();

	/** 다른 보안요원의 포획 완료 메시지를 받아 Captor/비-Captor 상태를 정리한다. */
	void HandleTargetCaptured(AFTSecurityAIController* Controller, const FFTNPCReportPayloadStruct& Payload) const;

	/** 플레이어 탈출 메시지를 받아 조사/스턴 상태로 전환할 준비를 한다. */
	void HandleTargetEscaped(AFTSecurityAIController* Controller, const FFTNPCReportPayloadStruct& Payload) const;
};
