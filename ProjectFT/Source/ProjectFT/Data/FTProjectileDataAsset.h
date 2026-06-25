#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/Struct/FTProjectileAttackStruct.h"
#include "FTProjectileDataAsset.generated.h"

// 투사체 공격용 아이템 데이터 에셋
// AFTItemActor를 상속받아서 값을 사용한다.
// ProjectileAttackData에 몽타주와 발사할 ProjectileActor의 정보를 넣는다.
UCLASS(BlueprintType)
class PROJECTFT_API AFTProjectileDataAsset : public AFTItemActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Data")
	FFTProjectileAttackStruct ProjectileAttackData;
};
