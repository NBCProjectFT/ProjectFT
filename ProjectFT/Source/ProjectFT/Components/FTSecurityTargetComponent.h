#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTSecurityTargetComponent.generated.h"

class AFTSecurityAIController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityTargetComponent();

	/** 타겟이 변경되었을 때 이전 시야 기억을 초기화한다. */
	void ResetTargetMemory();

	/** 보안요원의 StateTree가 사용할 타겟 거리, 시야, 공격 범위 상태를 계산한다. */
	void UpdateTargetState(
		const AFTSecurityAIController* Controller,
		AActor* TargetActor,
		bool bInTargetCurrentlyVisible,
		float AttackRange,
		float TargetSightLostGracePeriod,
		float& OutTargetDistance,
		bool& bOutHasSeenTarget,
		bool& bOutIsTargetInAttackRange);

	/** 이번 갱신에서 타겟을 실제로 볼 수 있었는지 반환한다. */
	bool WasTargetCurrentlyVisible() const;

private:
	float LastTargetVisibleTime = -BIG_NUMBER;
	bool bTargetCurrentlyVisible = false;
};
