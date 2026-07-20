#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Security/FTSecurityAIController.h"
#include "FTAttackSecurityAIController.generated.h"

class UGameplayEffect;

UCLASS()
class PROJECTFT_API AFTAttackSecurityAIController : public AFTSecurityAIController
{
	GENERATED_BODY()

public:
	AFTAttackSecurityAIController();

	/** StateTree의 Attack 상태 진입 시 호출해 공격 상태를 시작한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Security|Attack")
	bool StartAttack();

	/** 공격 애니메이션이나 공격 대기 시간이 끝났을 때 호출해 공격 상태를 종료한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Security|Attack")
	void FinishAttack();

	/** 현재 공격을 시작할 수 있는 상태인지 반환한다. */
	UFUNCTION(BlueprintPure, Category = "FT|Security|Attack")
	bool CanStartAttack() const;

	/** 공격 타이밍에 대상이 아직 공격 범위 안에 있으면 데미지 GE를 적용한다. */
	UFUNCTION(BlueprintCallable, Category = "FT|Security|Attack")
	bool ApplyAttackDamage();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Attack")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Security|Attack")
	bool bAttackCompleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Attack", meta = (ClampMin = "0.0"))
	float AttackCooldown = 1.0f;

	/** 공격 한 번에 적용할 데미지 양이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Attack", meta = (ClampMin = "0.0"))
	float AttackDamage = 10.0f;

	/** 공격 데미지에 사용할 GameplayEffect다. 기본값은 UFTGE_Damage다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Security|Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	float LastAttackFinishedTime = -BIG_NUMBER;
};
