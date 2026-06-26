#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FTMeleeActionTraceNotifyState.generated.h"

class UMeshComponent;
struct FFTMeleeActionStruct;

UCLASS(meta = (DisplayName = "FT Melee Attack Trace"))
class PROJECTFT_API UFTMeleeActionTraceNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	UMeshComponent* ResolveTraceMesh(
		const USkeletalMeshComponent* MeshComp,
		FName InHitStartSocketName,
		FName InHitEndSocketName
	) const;

protected:
	// 무기 또는 캐릭터 Mesh에 있어야 하는 시작 소켓
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace")
	FName HitStartSocketName = TEXT("Hit_Start");

	// 무기 또는 캐릭터 Mesh에 있어야 하는 끝 소켓
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace")
	FName HitEndSocketName = TEXT("Hit_End");

	// 캡슐 판정 반지름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace", meta = (ClampMin = "1.0"))
	float CapsuleRadius = 18.0f;

	// 어떤 채널로 Overlap할지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// 디버그 표시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Debug")
	bool bDrawDebug = true;

private:
	const FFTMeleeActionStruct* ResolveMeleeActionData(
		const USkeletalMeshComponent* MeshComp
	) const;

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

	void SendTraceBeginEvent(USkeletalMeshComponent* MeshComp) const;
	void SendTraceEndEvent(USkeletalMeshComponent* MeshComp) const;
	void TraceAndSendHitEvent(USkeletalMeshComponent* MeshComp) const;
};