// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectFT/Enum/FTSkillCheckResultType.h"
#include "FTChanneledInteractionComponent.generated.h"

// 채널링(게이지 진행) 시작/종료. UI 게이지 표시·숨김 등에 사용.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTOnChannelStateChanged, bool, bIsChanneling);
// 게이지가 가득 차 작업이 완료됨. 대상(소유 액터)이 바인딩해 완료 효과를 처리한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnChannelCompleted);
// 스킬체크 시작. 커서는 0에서 시작해 SweepSeconds 동안 1까지 증가한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnSkillCheckStarted);
// 스킬체크 종료(결과 포함).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTOnSkillCheckEnded, EFTSkillCheckResultType, Result);

/**
 * 데드 바이 데이라이트의 발전기처럼 "꾹 눌러 게이지를 채우는" 채널형 상호작용 대상에 부착한다.
 * 진행도(Progress)를 대상이 보유하고, 채널링 중 주기적으로 스킬체크를 띄운다.
 * 플레이어 입력 자체는 받지 않는다 — 플레이어의 UFTInteractionComponent가 Start/Stop/NotifySkillCheckInput을 호출한다.
 * 연속값(진행도·커서)은 getter로 폴링하고(UMG 바인딩 권장), 이산 이벤트는 델리게이트로 받는다.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTChanneledInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTChanneledInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 플레이어가 상호작용 키를 누르기 시작 → 채널링 시작(이미 진행 중이면 무시).
	void StartChannel(AActor* InInteractor);

	// 키를 떼거나 범위를 벗어남 → 채널링 중단(진행도는 유지된다).
	void StopChannel();

	// 스킬체크 입력. 활성 스킬체크가 있으면 현재 커서 위치로 판정한다.
	void NotifySkillCheckInput();

	//~ UI 폴링용 getter (연속값)
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetProgress() const { return Progress; }
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") bool IsChanneling() const { return bIsChanneling; }
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") bool IsSkillCheckActive() const { return bSkillCheckActive; }
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetSkillCheckCursor() const { return SkillCheckCursor; }
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetSkillCheckTarget() const { return SkillCheckTarget; }
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetSkillCheckSuccessHalfWidth() const { return SkillCheckSuccessHalfWidth; }
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetSkillCheckGreatHalfWidth() const { return SkillCheckGreatHalfWidth; }

	//~ 이벤트(델리게이트)
	UPROPERTY(BlueprintAssignable, Category = "FT|Interaction") FFTOnChannelStateChanged OnChannelStateChanged;
	UPROPERTY(BlueprintAssignable, Category = "FT|Interaction") FFTOnChannelCompleted OnCompleted;
	UPROPERTY(BlueprintAssignable, Category = "FT|Interaction") FFTOnSkillCheckStarted OnSkillCheckStarted;
	UPROPERTY(BlueprintAssignable, Category = "FT|Interaction") FFTOnSkillCheckEnded OnSkillCheckEnded;

protected:
	// 0→1까지 채우는 데 필요한 총 작업 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction", meta = (ClampMin = "0.01"))
	float RequiredSeconds = 8.0f;

	// 스킬체크 사이 간격(초) 범위. 채널 시작/스킬체크 종료 후 이 범위에서 랜덤으로 다음 스킬체크를 예약한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.0"))
	float SkillCheckIntervalMin = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.0"))
	float SkillCheckIntervalMax = 6.0f;

	// 커서가 0→1까지 이동하는 시간(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.05"))
	float SkillCheckSweepSeconds = 1.0f;

	// 성공(Good) 판정 반폭. 성공존 = [타깃-반폭, 타깃+반폭]. 0.06이면 폭 0.12(약 ±60ms@1s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float SkillCheckSuccessHalfWidth = 0.06f;

	// 대성공(Great) 판정 반폭(성공존 안의 더 좁은 중앙존). 0이면 Great 비활성.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float SkillCheckGreatHalfWidth = 0.02f;

	// 타깃 난수 범위. 반응 시간 확보용. 성공존이 0..1 안에 들어오도록 내부에서 추가 클램프된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SkillCheckTargetMin = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SkillCheckTargetMax = 0.9f;

	// Miss 시 잃는 진행도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MissProgressPenalty = 0.1f;

	// Great 시 얻는 보너스 진행도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GreatProgressBonus = 0.03f;

	// Miss 시 채널링을 중단할지(true면 다시 눌러야 재개). DBD는 false(계속 진행 + 회귀).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction|SkillCheck")
	bool bMissStopsChannel = false;

private:
	void TickChannel(float DeltaTime);
	void StartSkillCheck();
	void EndSkillCheck(EFTSkillCheckResultType Result);
	EFTSkillCheckResultType EvaluateSkillCheckAtCursor() const;
	void ScheduleNextSkillCheck();
	void SetProgress(float NewProgress);
	void CompleteChannel();

	// 현재 채널링 중인 액터(보통 플레이어).
	UPROPERTY(Transient)
	TObjectPtr<AActor> Interactor;

	// 작업 진행도(0..1). 채널링을 멈춰도 유지된다.
	float Progress = 0.0f;
	bool bIsChanneling = false;

	// 스킬체크 상태.
	bool bSkillCheckActive = false;
	float SkillCheckCursor = 0.0f;   // 0→1로 증가
	float SkillCheckTarget = 0.0f;   // 성공존 중심
	float TimeUntilNextSkillCheck = 0.0f;
};
