#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "FTWeaponStruct.generated.h"

class UAnimMontage;
class UTexture2D;

// Weapon들의 정보를 담아 놓을 DataTable로 사용할 Struct

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTWeaponStruct : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	FName ItemName = NAME_None;

	// Melee, HitScan, Projectile 등의 Tag를 넣어놓을 곳
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	FGameplayTag AttackTypeTag;

	// Player가 재생할 몽타주를 넣어서 사용
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	// TSoftObjectPtr<UAnimMontage> AnimMontage;

	// TODO : 여기서 아이템 능력치 구현
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|UI")
	TSoftObjectPtr<UTexture2D> ItemImage;
};
