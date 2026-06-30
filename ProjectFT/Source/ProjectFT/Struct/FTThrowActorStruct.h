#pragma once

#include "CoreMinimal.h"
#include "FTThrowActorStruct.generated.h"

class UAnimMontage;
class UFTProjectileActorDataAsset;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTThrowActorStruct
{
	GENERATED_BODY()

public:
	// 사용 입력을 누른 직후, 투사체를 손에 들고 준비 자세를 보여줄 때 재생할 몽타주.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Animation")
	TObjectPtr<UAnimMontage> PrepareMontage = nullptr;

	// 사용 입력을 놓은 뒤 실제 던지는 동작을 보여줄 몽타주.
	// 이 몽타주 안에 UFTThrowReleaseAnimNotify를 배치해야 실제 발사 타이밍이 맞는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Animation")
	TObjectPtr<UAnimMontage> ThrowMontage = nullptr;

	// 손에 들었다가 발사할 투사체 아이템 데이터.
	// 이 데이터의 ProjectileActorData가 실제 스폰 클래스, 속도, 충돌/폭발 설정을 제공한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Projectile")
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileItemData = nullptr;

	// 준비 상태에서 ProjectileActor를 붙일 캐릭터/무기 Mesh 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw|Attach")
	FName AttachSocketName = TEXT("MeleeHandGrip_R");
};
