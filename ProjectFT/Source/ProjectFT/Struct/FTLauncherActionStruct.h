#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/Data/FTProjectileActorDataAsset.h"
#include "FTLauncherActionStruct.generated.h"

class UAnimMontage;
class UFTItemDataAsset;

/**
 * 발사기 아이템이 UFTGA_LauncherAction을 실행할 때 사용하는 데이터다.
 *
 * 발사기 아이템 자체의 UseData는 발사 비용/쿨다운에 쓰이고,
 * ProjectileItemData는 실제로 스폰될 투사체 액터와 적중 효과를 정의한다.
 */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTLauncherActionStruct
{
	GENERATED_BODY()

public:
	// 발사 입력 시 재생할 몽타주.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launcher|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;
	
	// 발사할 투사체 데이터. 이 데이터의 ProjectileActorData가 스폰 클래스/속도/충돌을 제공한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launcher|ProjectileItemData")
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileItemData;

	// 발사기 아이템을 캐릭터 손에 장착할 때 사용할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launcher|Attach")
	FName AttachSocketName = TEXT("MeleeHandGrip_R");
	
	// ProjectileActor를 스폰할 총구 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launcher|Fire")
	FName MuzzleSocketName = TEXT("Muzzle");
};
