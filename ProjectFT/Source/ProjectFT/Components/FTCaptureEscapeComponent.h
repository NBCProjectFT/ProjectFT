// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectFT/Struct/FTStruggleGaugeStruct.h"
#include "FTCaptureEscapeComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

// 게이지가 가득 차 탈출에 성공한 순간(1회). 붙잡은 어빌리티(UFTGA_Grab)가 바인딩해 해방/스턴 처리를 한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnCaptureEscaped);

/**
 * 플레이어가 경비에게 "붙잡힌" 상태를 소유하는 컴포넌트(두-바디 상호작용).
 *  - BeginCapture: State.Captured/State.Escapable 부여 + 붙잡은 액터의 지점(CapturePoint)에 attach + 이동/충돌 정지
 *  - 좌우 연타 탈출: 플레이어 입력 → Event.Struggle → 공용 게이지(FTStruggleGaugeStruct)가 채워지고, 가득 차면 OnEscaped 통지
 *  - EndCapture: 부착 해제 + 이동/충돌/태그 원복
 *
 * 탈출 게이지 '계산'은 공용 FTStruggleGaugeStruct에 위임한다(잡기·비눗방울 공용). 이 컴포넌트는 두-바디 상태와
 * 게이지 구동만 담당하고, 성공(스턴)/실패(피해)/수명은 붙잡은 어빌리티(UFTGA_Grab)가 관장한다.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTCaptureEscapeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTCaptureEscapeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 붙잡힘 시작. InCaptor=붙잡은 액터, InAttachPoint=추종할 부착 지점(경비 CapturePoint),
	// InEscapeThreshold=탈출에 필요한 총 struggle 양(= 경비의 붙잡는 힘), InDecayPerSecond=초당 되끌어내리는 힘(= AI의 탈출 저지력).
	// 둘 다 붙잡은 어빌리티 UFTGA_Grab이 주입한다(공용 게이지에 그대로 전달). 이미 붙잡힌 상태면 무시.
	bool TryBeginCapture(AActor* InCaptor, USceneComponent* InAttachPoint, float InEscapeThreshold = 1.0f, float InDecayPerSecond = 0.0f);

	// 붙잡힘 해제(어빌리티가 성공/실패/취소 어느 경로로든 호출). 이동/충돌/부착/태그를 원복한다.
	void EndCapture();

	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	bool IsCaptured() const { return bCaptured; }

	/** Returns the actor that currently owns this capture. */
	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	AActor* GetCaptorActor() const { return Captor.Get(); }

	// UI용 탈출 진행 비율(0..1). 임계값으로 정규화돼 임계값이 달라도 게이지는 0~1.
	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	float GetEscapeProgress() const { return Gauge.GetProgress(); }

	// 현재 누적 struggle(절대값)과 탈출 임계값(AI 붙잡는 힘). 디버깅/연출용.
	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	float GetAccumulatedStruggle() const { return Gauge.GetAccumulated(); }

	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	float GetEscapeThreshold() const { return Gauge.GetThreshold(); }

	UPROPERTY(BlueprintAssignable, Category = "FT|Capture")
	FFTOnCaptureEscaped OnEscaped;

protected:
	// 좌우 연타 탈출 게이지(능동 GainPerFlip · 수동 PassiveGain). 임계값/저지력은 TryBeginCapture로 주입된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Capture")
	FTStruggleGaugeStruct Gauge;

private:
	// Event.Struggle(플레이어 발버둥 입력) 수신 → 게이지 1회 상승. 가득 차면 탈출 처리.
	void OnStruggleEvent(const FGameplayEventData* Payload);

	// 게이지가 가득 찼으면 1회만 OnEscaped를 통지한다.
	void TryComplete();

	UAbilitySystemComponent* GetOwnerAbilitySystem() const;

	bool bCaptured = false;
	bool bEscaped = false;

	// Event.Struggle 리스너 해제용 핸들.
	FDelegateHandle StruggleEventHandle;

	// 원복용: 붙잡히기 전 캡슐 콜리전 설정.
	TEnumAsByte<ECollisionEnabled::Type> SavedCollisionEnabled = ECollisionEnabled::QueryAndPhysics;

	// 원복용: 붙잡히기 전 컨트롤러 yaw 추종 설정.
	bool bSavedUseControllerRotationYaw = true;

	// 원복용: 붙잡은 경비(캡터)의 카메라 채널(ECC_Camera) 응답.
	bool bCaptorCameraResponseSaved = false;
	TEnumAsByte<ECollisionResponse> SavedCaptorCapsuleCameraResponse = ECR_Block;
	TEnumAsByte<ECollisionResponse> SavedCaptorMeshCameraResponse = ECR_Block;

	TWeakObjectPtr<AActor> Captor;
};
