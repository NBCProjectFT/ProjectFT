// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_ItemAbility.h"
#include "FTGA_ThrowItem.generated.h"

struct FGameplayEventData;

/**
 * [전용 로직 어빌리티] 들고 있는 아이템을 "꾹 눌러 조준하고 손을 떼면" 시점 정면으로 던진다(충전형).
 *  - 발동(누름)  : 조준 시작(궤적 프리뷰). 비용/쿨다운은 아직 커밋하지 않는다.
 *  - 발사(뗌)    : Event.UseReleased 수신 → 커밋 + 투사체 발사 + 소비 + 종료.
 *  - 취소(이동/퀵슬롯 전환/피격 등) : 던지지 않고 종료 — 효과/쿨다운 미적용, 궤적 프리뷰는 자동 정리.
 *
 *  - "무엇을 적용하나"(적중 시 효과) = 데이터(UseData.UseEffects)  ← 베이스(ApplyUseEffects)가 처리
 *  - "어떻게 던지나"(투사체 스폰 + 발사) = 이 GA만의 고유 로직     ← GE로 표현 불가라 전용 GA
 */
UCLASS()
class PROJECTFT_API UFTGA_ThrowItem : public UFTGA_ItemAbility
{
	GENERATED_BODY()

public:
	UFTGA_ThrowItem();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 종료(정상/취소/중단 전부)에서 조준 프리뷰를 정리한다 → 프리뷰는 어빌리티 수명에 묶인다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 조준 중 발사 지점/방향(시점=카메라 기준). BP가 궤적 프리뷰를 매 프레임 그릴 때 호출한다.
	UFUNCTION(BlueprintPure, Category = "FT|Throw")
	void GetThrowLaunchPoint(FVector& OutLocation, FRotator& OutRotation) const;

protected:
	// 조준 시작/종료 훅(궤적 프리뷰 표시/숨김). 비주얼은 BP에서 구현. OnAimStopped는 정상/취소/중단 모두에서 호출된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Throw")
	void OnAimStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Throw")
	void OnAimStopped();

	// 사용 입력에서 손을 뗀 순간(Event.UseReleased) 수신 콜백 → 실제 투척.
	UFUNCTION()
	void HandleReleaseEvent(FGameplayEventData Payload);

	// 실제 투척: 커밋(쿨다운/비용) → 투사체 발사 → 소비 → 종료.
	void PerformThrow();

	// 던지는 초기 속도(cm/s). BP로 파생해 아이템 종류별로 조정 가능. (테이저의 TraceRange와 같은 패턴)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Throw", meta = (ClampMin = "0.0"))
	float ThrowSpeed = 1200.0f;
};
