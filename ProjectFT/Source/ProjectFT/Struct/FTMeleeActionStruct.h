#pragma once

#include "CoreMinimal.h"
#include "FTMeleeActionStruct.generated.h"

class UAnimMontage;
class USoundBase;

/**
 * 근접 무기 아이템이 UFTGA_MeleeAction을 실행할 때 사용하는 데이터다.
 *
 * 데미지/상태이상 같은 GameplayEffect 정보는 부모 ItemData.UseData에 두고,
 * 이 구조체에는 몽타주, 손 장착 소켓, 근접 판정 캡슐 설정만 둔다.
 */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTMeleeActionStruct
{
	GENERATED_BODY()

	// 공격 입력 시 재생할 근접 공격 몽타주.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Animation")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	// 무언가를 실제로 때렸을 때 맞은 대상 위치에서 재생할 타격음. 비우면 무음.
	// 판정 구간(Trace Begin~End)당 '한 번만' 재생된다 — 한 번 휘둘러 여러 명을 쓸어도 소리는 하나다.
	// 휘두르는 소리(헛스윙해도 나는 소리)는 이게 아니라 부모 ItemData.UseData.UseSound 쪽이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Audio")
	TObjectPtr<USoundBase> HitSound = nullptr;

	// 아이템을 캐릭터 손에 장착할 때 사용할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Attach")
	FName AttachSocketName = TEXT("MeleeHandGrip_R");

	// 근접 판정 캡슐의 시작점으로 사용할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
	FName HitStartSocketName = TEXT("Hit_Start");

	// 근접 판정 캡슐의 끝점으로 사용할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
	FName HitEndSocketName = TEXT("Hit_End");

	// HitStartSocketName과 HitEndSocketName 사이를 감싸는 캡슐의 반지름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace", meta = (ClampMin = "1.0"))
	float CapsuleRadius = 18.0f;

	// 근접 Overlap 판정에 사용할 Collision Channel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// true면 AnimNotifyState가 판정 캡슐을 디버그로 그린다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Debug")
	bool bDrawDebug = true;
};
