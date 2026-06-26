#pragma once

#include "CoreMinimal.h"
#include "FTMeleeActionStruct.generated.h"

class UAnimMontage;

// 근접 공격에 필요한 연출/판정 데이터다.
// 데미지 GE와 SetByCaller 값은 부모 ItemData.UseData를 그대로 사용하고,
// 이 구조체는 몽타주와 타격 캡슐 정보만 담당한다.
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTMeleeActionStruct
{
	GENERATED_BODY()

	// 근접 공격 때 재생할 몽타주다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	// 어태치될 소켓의 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Attach")
	FName AttachSocketName = TEXT("MeleeHandGrip_R");

	// 타격 판정 캡슐의 시작 소켓이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
	FName HitStartSocketName = TEXT("Hit_Start");

	// 타격 판정 캡슐의 끝 소켓이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
	FName HitEndSocketName = TEXT("Hit_End");

	// 타격 판정 캡슐의 반지름이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace", meta = (ClampMin = "1.0"))
	float CapsuleRadius = 18.0f;

	// 오버랩 판정에 사용할 충돌 채널이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// 타격 판정 디버그 표시 여부다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Debug")
	bool bDrawDebug = true;
};
