#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_DamageItemAbility.generated.h"

/**
 * 데미지를 줄 수 있는 아이템 어빌리티의 공용 베이스.
 * ASC 대상은 UseEffects(GE) 경로가 처리하고, ASC가 없는 IFTDamageable 대상은 Actor Damage 경로로 처리한다.
 */
UCLASS(Abstract)
class PROJECTFT_API UFTGA_DamageItemAbility : public UFTGA_ItemAbility
{
	GENERATED_BODY()

protected:
	float ResolveActiveDamageAmount() const;
	bool ApplyDamageToDamageableTarget(AActor* TargetActor, const FHitResult* HitResult = nullptr) const;
};
