#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectFT/Struct/FTItemUseStruct.h"
#include "FTUseDataEffectComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTUseDataEffectExecuted, int32, AppliedEffectCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTUseDataEffectFailed, FName, Reason);

/**
 * 아이템이 아닌 액터가 FTItemUseStruct 기반 GameplayEffect 묶음을 실행할 때 붙이는 컴포넌트.
 *
 * 아이템 DA를 요구하지 않는다. 소유 액터/블루프린트가 UseData를 직접 보유하고,
 * 대상 액터의 ASC에 UseEffects + EffectMagnitudes를 적용한다.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTUseDataEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTUseDataEffectComponent();

	UFUNCTION(BlueprintCallable, Category = "FT|UseData")
	bool TryExecuteUseData(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "FT|UseData")
	void CancelPendingUse();

	UFUNCTION(BlueprintPure, Category = "FT|UseData")
	bool IsCasting() const { return bCasting; }

	UFUNCTION(BlueprintPure, Category = "FT|UseData")
	bool IsOnCooldown() const;

	UPROPERTY(BlueprintAssignable, Category = "FT|UseData")
	FFTUseDataEffectExecuted OnExecuted;

	UPROPERTY(BlueprintAssignable, Category = "FT|UseData")
	FFTUseDataEffectFailed OnFailed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|UseData")
	FTItemUseStruct UseData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|UseData")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	// 소유 액터가 ASC를 갖고 있지 않은 경우, 컴포넌트 내부 타이머로 쿨다운을 처리한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|UseData")
	bool bUseLocalCooldownWhenOwnerHasNoASC = true;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UAbilitySystemComponent* GetOwnerASC() const;
	UAbilitySystemComponent* GetTargetASC(AActor* TargetActor) const;
	bool CanStartUse(UAbilitySystemComponent* OwnerASC) const;
	void FinishExecuteUseData();
	void ApplyCooldown(UAbilitySystemComponent* OwnerASC);
	void Fail(FName Reason);

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PendingTargetActor;

	FTimerHandle CastTimerHandle;
	float LocalCooldownEndTime = 0.0f;
	bool bCasting = false;
};
