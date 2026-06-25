// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FTMeleeAttackTraceNotifyState.generated.h"

class UFTGA_MeleeAttack;
class UCapsuleComponent;
class UMeshComponent;

/**
 * MeleeAttack Ability의 실제 타격 판정 구간을 여는 AnimNotifyState입니다.
 *
 * 이 NotifyState는 판정 데이터나 데미지를 직접 들고 있지 않습니다.
 * 애니메이션 타임라인에서 "지금부터 지금까지 타격 판정을 검사한다"는 신호만 보내고,
 * 캡슐 위치 계산, Debug Draw, 중복 히트 방지, GameplayEffect 적용은 UMeleeAttack이 처리합니다.
 */
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

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace")
	FName HitStartSocketName = TEXT("Hit_Start");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace")
	FName HitEndSocketName = TEXT("Hit_End");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Trace", meta = (ClampMin = "1.0"))
	float CapsuleRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Debug")
	bool bDrawFallbackWhenSocketsMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Debug")
	bool bCreateTraceCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Melee|Debug", meta = (ClampMin = "1.0"))
	float FallbackCapsuleLength = 110.0f;

private:
	UFTGA_MeleeAttack* FindActiveMeleeAttackAbility(const USkeletalMeshComponent* MeshComp) const;
	void UpdateTraceCollision(const USkeletalMeshComponent* MeshComp, bool bLogMissingSockets);
	void DestroyTraceCollision(const USkeletalMeshComponent* MeshComp);
	UCapsuleComponent* FindOrCreateTraceCollision(const USkeletalMeshComponent* MeshComp);
	const UMeshComponent* ResolveTraceMesh(const USkeletalMeshComponent* MeshComp) const;
	bool BuildTraceCapsule(const USkeletalMeshComponent* MeshComp, FVector& OutStart, FVector& OutEnd, float& OutHalfHeight, FQuat& OutRotation, bool bLogMissingSockets) const;

	TMap<uint32, TWeakObjectPtr<UCapsuleComponent>> ActiveTraceCapsules;
};
