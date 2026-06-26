#pragma once

#include "CoreMinimal.h"
#include "FTItemDataAsset.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Struct/FTProjectileActionStruct.h"
#include "FTProjectileDataAsset.generated.h"

// 투사체 공격용 아이템 데이터 에셋
// AFTItemActor를 상속받아서 값을 사용한다.
// ProjectileAttackData에 몽타주와 발사할 ProjectileActor의 정보를 넣는다.
UCLASS(BlueprintType)
class PROJECTFT_API UFTProjectileDataAsset : public UFTItemDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile Data")
	FFTProjectileActionStruct ProjectileAttackData;
};
