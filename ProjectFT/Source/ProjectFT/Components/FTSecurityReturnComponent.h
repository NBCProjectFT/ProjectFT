#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "FTSecurityReturnComponent.generated.h"

class AFTSecurityAIController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityReturnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityReturnComponent();

	/** 맵에 배치된 보안요원의 원래 위치와 방향을 저장한다. */
	void InitializeHome(AFTSecurityAIController* Controller, APawn* ControlledPawn) const;

	/** 보안실 복귀 지점 근처에서 보안요원끼리 막히지 않도록 충돌을 임시 무시한다. */
	void UpdateReturnCollision(AFTSecurityAIController* Controller) const;

	/** 복귀 이동 완료 결과를 검사하고 실제 복귀 완료 여부를 판정한다. */
	void HandleMoveCompleted(AFTSecurityAIController* Controller, const FPathFollowingResult& Result) const;

	/** 복귀 완료 후 배치 보안요원은 원래 방향으로 복구하고, 소환 보안요원은 보안실에 복귀 이벤트를 발행한다. */
	void CompleteReturn(AFTSecurityAIController* Controller) const;
};
