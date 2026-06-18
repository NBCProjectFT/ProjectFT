#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTAttackComponent.generated.h"

// AttackComponent들의 최상위 컴포넌트 여기서 각종 효과들 구현
UCLASS(Abstract, Blueprintable, ClassGroup = (FT))
class PROJECTFT_API UFTAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTAttackComponent();

	UFUNCTION(BlueprintCallable, Category = "FT|Weapon")
	virtual void Attack();
};
