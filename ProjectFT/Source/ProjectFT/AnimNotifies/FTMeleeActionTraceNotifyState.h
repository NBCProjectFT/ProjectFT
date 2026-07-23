#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FTMeleeActionTraceNotifyState.generated.h"

class UMeshComponent;
struct FFTMeleeActionStruct;

/**
 * 근접 공격 몽타주 안에서 실제 타격 판정 구간을 표시하는 AnimNotifyState다.
 *
 * NotifyBegin에서 Ability에 Trace Begin 이벤트를 보내고,
 * NotifyTick마다 무기/캐릭터 소켓 사이를 캡슐 형태로 Overlap 검사한 뒤 Hit 이벤트를 보낸다.
 * NotifyEnd에서는 Trace End 이벤트를 보내 UFTGA_MeleeAction이 판정 상태를 닫게 한다.
 */
UCLASS(meta = (DisplayName = "FT Melee Attack Trace"))
class PROJECTFT_API UFTMeleeActionTraceNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 판정 구간 시작. MeleeAction에 Trace Begin 이벤트를 보낸다.
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	// 판정 구간 동안 매 프레임 소켓 위치를 따라 캡슐 Overlap을 수행한다.
	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	// 판정 구간 종료. MeleeAction에 Trace End 이벤트를 보낸다.
	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 무기 액터 또는 캐릭터 Mesh 중 HitStart/HitEnd 소켓을 가진 Mesh를 찾는다.
	UMeshComponent* ResolveTraceMesh(
		const USkeletalMeshComponent* MeshComp,
		FName InHitStartSocketName,
		FName InHitEndSocketName
	) const;

protected:
	// 판정 캡슐의 시작점으로 사용할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace")
	FName HitStartSocketName = TEXT("Hit_Start");

	// 판정 캡슐의 끝점으로 사용할 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace")
	FName HitEndSocketName = TEXT("Hit_End");

	// HitStart/HitEnd 사이를 감싸는 캡슐의 반지름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace", meta = (ClampMin = "1.0"))
	float CapsuleRadius = 18.0f;

	// 어떤 Collision Channel을 대상으로 Overlap할지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// true면 몽타주 재생 중 캡슐 판정 위치를 Debug Draw로 표시한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Debug")
	bool bDrawDebug = true;

private:
	struct FPreviousTraceFrame
	{
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
	};

	// AnimNotifyState는 공유되므로 재생 중인 메시마다 이전 프레임 위치를 따로 보관한다.
	mutable TMap<TWeakObjectPtr<USkeletalMeshComponent>, FPreviousTraceFrame> PreviousTraceFrames;

	// 현재 캐릭터가 들고 있는 아이템에서 MeleeActionData를 가져온다.
	const FFTMeleeActionStruct* ResolveMeleeActionData(
		const USkeletalMeshComponent* MeshComp
	) const;

	// 두 소켓 위치를 기준으로 캡슐 Overlap에 필요한 중심/길이/회전을 계산한다.
	bool BuildTraceCapsule(
		const UMeshComponent* TraceMesh,
		FName InHitStartSocketName,
		FName InHitEndSocketName,
		float InCapsuleRadius,
		FVector& OutStart,
		FVector& OutEnd,
		FVector& OutCenter,
		float& OutHalfHeight,
		FQuat& OutRotation
	) const;

	// Ability에 근접 판정 시작 이벤트를 보낸다.
	void SendTraceBeginEvent(USkeletalMeshComponent* MeshComp) const;

	// Ability에 근접 판정 종료 이벤트를 보낸다.
	void SendTraceEndEvent(USkeletalMeshComponent* MeshComp) const;

	// 실제 Overlap을 수행하고 맞은 액터들을 Ability에 Hit 이벤트로 전달한다.
	void TraceAndSendHitEvent(USkeletalMeshComponent* MeshComp) const;
};
