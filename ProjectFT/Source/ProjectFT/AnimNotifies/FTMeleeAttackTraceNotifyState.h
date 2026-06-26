
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FTMeleeAttackTraceNotifyState.generated.h"

class UMeshComponent;

UCLASS(meta = (DisplayName = "FT Melee Attack Trace"))
class PROJECTFT_API UFTMeleeAttackTraceNotifyState : public UAnimNotifyState
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

	UMeshComponent* ResolveTraceMesh(const USkeletalMeshComponent* MeshComp) const;

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
	
	bool BuildTraceCapsule(
		const UMeshComponent* TraceMesh,
		FVector& OutStart,
		FVector& OutEnd,
		FVector& OutCenter,
		float& OuthalfHeight,
		FQuat& OutRotation
		) const;

	void SendTraceBeginEvent(USkeletalMeshComponent* MeshComp) const;
	void SendTraceEndEvent(USkeletalMeshComponent* MeshComp) const;
	void TraceAndSendHitEvent(USkeletalMeshComponent* MeshComp) const;
};
