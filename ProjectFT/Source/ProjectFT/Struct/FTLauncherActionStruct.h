#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Data/FTProjectileActorDataAsset.h"
#include "FTLauncherActionStruct.generated.h"

class UAnimMontage;
class UFTItemDataAsset;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTLauncherActionStruct
{
	GENERATED_BODY()

public:
	// 공격 때 재생할 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launcher|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launcher|ProjectileItemData")
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileItemData;

	// 어태치될 소켓의 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launcher|Attach")
	FName AttachSocketName = TEXT("MeleeHandGrip_R");
	
	// 총구 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launcher|Fire")
	FName MuzzleSocketName = TEXT("Muzzle");
};
