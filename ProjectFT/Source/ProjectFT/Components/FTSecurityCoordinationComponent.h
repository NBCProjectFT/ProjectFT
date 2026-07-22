#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTSecurityCoordinationComponent.generated.h"

class AFTSecurityAIController;

/**
 * 보안요원 중 한 명을 접근 리더로 선정하고, 나머지 보안요원에게 안정적인 포위 위치를 배정한다.
 * StateTree는 bIsAttackLeader를 Approach 전환에 사용하고, EncircleSlotLocation을 포위 이동 위치로 사용한다.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTSecurityCoordinationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTSecurityCoordinationComponent();
	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** 보안요원 컨트롤러를 접근 리더 후보로 등록한다. */
	void RegisterSecurityController(AFTSecurityAIController* SecurityController);

	/** 보안요원 컨트롤러를 접근 리더 후보에서 제거한다. */
	void UnregisterSecurityController(AFTSecurityAIController* SecurityController);

	/** 현재 타겟에게 접근할 수 있는 보안요원 컨트롤러를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|Coordination")
	AFTSecurityAIController* GetAttackLeader() const;

protected:
	/** 기존 리더를 교체하려면 새 후보가 이 거리만큼 더 가까워야 한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination", meta = (ClampMin = "0.0"))
	float LeaderSwitchDistanceAdvantage = 30.0f;

	/** 접근 리더를 다시 평가하는 주기다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination", meta = (ClampMin = "0.05"))
	float SelectionInterval = 0.2f;

	/** 포위 위치를 만들 때 타겟으로부터 떨어질 거리다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination", meta = (ClampMin = "100.0"))
	float EncircleRadius = 550.0f;

	/** 생성된 포위 위치를 NavMesh 위로 투영할 때 사용하는 탐색 범위다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Coordination")
	FVector EncircleNavProjectionExtent = FVector(200.0f, 200.0f, 300.0f);

private:
	TSet<TWeakObjectPtr<AFTSecurityAIController>> RegisteredSecurityControllers;
	TWeakObjectPtr<AFTSecurityAIController> AttackLeader;

	bool IsEligibleAttackLeader(const AFTSecurityAIController* SecurityController) const;
	void UpdateAttackLeader();
	void UpdateEncircleSlots();
	void SetAttackLeader(AFTSecurityAIController* NewAttackLeader);
	void RemoveInvalidControllers();
};
