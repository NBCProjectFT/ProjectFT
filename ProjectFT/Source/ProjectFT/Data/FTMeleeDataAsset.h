#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Struct/FTMeleeActionStruct.h"
#include "FTMeleeDataAsset.generated.h"

// 근거리 동작 아이템 데이터 에셋이다.
// 부모의 ItemData.UseData에는 UseAbility, UseEffects, SetByCaller, 쿨다운을 넣고,
// MeleeActionData에는 몽타주와 타격 캡슐 정보를 넣는다.
UCLASS(BlueprintType)
class PROJECTFT_API UFTMeleeDataAsset : public UFTItemDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Data")
	FFTMeleeActionStruct MeleeActionData;
};
