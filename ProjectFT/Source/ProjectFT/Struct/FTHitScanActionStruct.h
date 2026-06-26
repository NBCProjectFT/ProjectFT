#pragma once

#include "CoreMinimal.h"
#include "FTHitScanActionStruct.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct PROJECTFT_API FFTHitScanActionStruct
{
	GENERATED_BODY()

public:
	// 공격 때 재생할 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	// 어태치될 소켓의 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Attach")
	FName AttachSocketName = TEXT("HitScanHandGrip_R");

	// 총구 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Trace")
	FName MuzzleSocketName = TEXT("Muzzle");

	// 히트스캔 판정에 사용될 충돌 판정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;
	
	// 사정거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Range")
	float Range = 4000.0f;

	// 판정 디버그 표시 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan|Debug")
	bool bDrawDebug = true;
};