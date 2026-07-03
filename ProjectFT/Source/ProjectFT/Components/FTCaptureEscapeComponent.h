// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FTCaptureEscapeComponent.generated.h"

class UAbilitySystemComponent;

// 게이지가 가득 차 탈출에 성공한 순간(1회). 붙잡은 어빌리티(UFTGA_Grab)가 바인딩해 해방/스턴 처리를 한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnCaptureEscaped);

/**
 * 플레이어가 경비에게 "붙잡힌" 상태를 소유하는 컴포넌트.
 *  - BeginCapture: State.Captured 부여 + 붙잡은 액터의 지점(CapturePoint)에 attach + 이동/충돌 정지(스스로 못 움직임)
 *  - 좌우 연타(이동 X축 방향 전환)로 탈출 게이지를 채우고, 가득 차면 OnEscaped 통지
 *  - EndCapture: 부착 해제 + 이동/충돌/태그 원복
 * 실제 성공(스턴)/실패(피해) 판정과 수명은 붙잡은 어빌리티가 관장한다 — 이 컴포넌트는 상태와 게이지만 담당.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTCaptureEscapeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTCaptureEscapeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 붙잡힘 시작. InCaptor=붙잡은 액터, InAttachPoint=추종할 부착 지점(경비 CapturePoint). 이미 붙잡힌 상태면 무시.
	void BeginCapture(AActor* InCaptor, USceneComponent* InAttachPoint);

	// 붙잡힘 해제(어빌리티가 성공/실패/취소 어느 경로로든 호출). 이동/충돌/부착/태그를 원복한다.
	void EndCapture();

	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	bool IsCaptured() const { return bCaptured; }

	// 좌우 연타 탈출 입력. 붙잡힘 중 플레이어의 이동 X축 값을 받아, 방향이 바뀔 때마다 게이지를 올린다.
	void AddStruggleInput(float MoveAxisX);

	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	float GetEscapeProgress() const { return EscapeProgress; }

	UPROPERTY(BlueprintAssignable, Category = "FT|Capture")
	FFTOnCaptureEscaped OnEscaped;

protected:
	// 좌우 방향 전환 1회로 채워지는 게이지 양(0..1). 예) 0.06 → 약 17번 전환이면 탈출.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Capture", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StruggleGainPerFlip = 0.06f;

	// 초당 자연 감소량(연타를 멈추면 게이지가 줄어 계속 연타를 강제). 0이면 감소 없음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Capture", meta = (ClampMin = "0.0"))
	float StruggleDecayPerSecond = 0.1f;

	// 방향 전환으로 인정할 최소 입력 크기(데드존). 작은 흔들림/노이즈 무시.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Capture", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StruggleInputDeadzone = 0.3f;

private:
	void SetProgress(float NewProgress);
	UAbilitySystemComponent* GetOwnerAbilitySystem() const;

	bool bCaptured = false;
	bool bEscaped = false;
	float EscapeProgress = 0.0f;

	// 마지막으로 인정된 입력 방향 부호(-1/0/+1). 부호가 바뀌면 flip으로 게이지 상승.
	float LastStruggleSign = 0.0f;

	// 원복용: 붙잡히기 전 캡슐 콜리전 설정.
	TEnumAsByte<ECollisionEnabled::Type> SavedCollisionEnabled = ECollisionEnabled::QueryAndPhysics;

	TWeakObjectPtr<AActor> Captor;
};
