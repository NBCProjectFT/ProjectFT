#pragma once

#include "CoreMinimal.h"
#include "FTHitScanActionStruct.generated.h"

class UAnimMontage;

/**
 * 즉발 판정 무기 아이템이 UFTGA_HitScanAction을 실행할 때 사용하는 데이터다.
 *
 * UseEffects는 부모 ItemData.UseData에 두고,
 * 여기에는 몽타주, 장착 소켓, 총구 소켓, Trace 판정 정보를 둔다.
 */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTHitScanActionStruct
{
	GENERATED_BODY()

public:
	// 공격 입력 시 재생할 몽타주.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	// 아이템을 캐릭터 손에 장착할 때 사용할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Attach")
	FName AttachSocketName = TEXT("HitScanHandGrip_R");

	// 실제 판정 Trace를 시작할 총구 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Trace")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 히트스캔 LineTrace에 사용할 Collision Channel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;
	
	// 카메라 조준 방향으로 검사할 최대 사거리.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Range")
	float Range = 4000.0f;

	// true면 Muzzle에서 조준점까지의 Trace 선을 디버그로 그린다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Debug")
	bool bDrawDebug = true;
};
