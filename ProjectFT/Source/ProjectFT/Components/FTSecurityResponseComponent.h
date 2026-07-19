#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FTSecurityResponseComponent.generated.h"

class AFTSecurityAIController;
struct FFTCharacterAttackedPayloadStruct;
struct FFTMessagePayloadStruct;
struct FFTNPCReportPayloadStruct;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityResponseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityResponseComponent();

	/** 신고 메시지를 받았을 때 조사/추격 상태로 진입할 준비를 한다. */
	void HandleSecurityCalled(AFTSecurityAIController* Controller, const FFTNPCReportPayloadStruct& Payload) const;

	/** 매대 파손을 직접 목격했을 때 플레이어 추격 상태로 진입할 준비를 한다. */
	void HandleShelfDamaged(AFTSecurityAIController* Controller, const FFTMessagePayloadStruct& Payload) const;

	/** 보안요원이 직접 맞았거나 폭행을 목격했을 때 플레이어 추격 상태로 진입할 준비를 한다. */
	void HandleCharacterAttacked(AFTSecurityAIController* Controller, const FFTCharacterAttackedPayloadStruct& Payload) const;
};
