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

	// 붙잡힘 시작. InCaptor=붙잡은 액터, InAttachPoint=추종할 부착 지점(경비 CapturePoint),
	// InEscapeThreshold=탈출에 필요한 총 struggle 양(= 경비의 붙잡는 힘), InDecayPerSecond=초당 되끌어내리는 힘(= AI의 탈출 저지력).
	// 둘 다 붙잡은 어빌리티 UFTGA_Grab이 주입한다. 이미 붙잡힌 상태면 무시.
	/** Attempts to reserve this target for one captor and starts the captured state. */
	bool TryBeginCapture(AActor* InCaptor, USceneComponent* InAttachPoint, float InEscapeThreshold = 1.0f, float InDecayPerSecond = 0.0f);

	// 붙잡힘 해제(어빌리티가 성공/실패/취소 어느 경로로든 호출). 이동/충돌/부착/태그를 원복한다.
	void EndCapture();

	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	bool IsCaptured() const { return bCaptured; }

	/** Returns the actor that currently owns this capture. */
	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	AActor* GetCaptorActor() const { return Captor.Get(); }

	// 좌우 연타 탈출 입력. 붙잡힘 중 플레이어의 이동 X축 값을 받아, 방향이 바뀔 때마다 게이지를 올린다.
	void AddStruggleInput(float MoveAxisX);

	// UI용 탈출 진행 비율(0..1). 누적 struggle을 임계값(AI 붙잡는 힘)으로 나눈 값이라 임계값이 달라도 게이지는 0~1로 정규화된다.
	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	float GetEscapeProgress() const { return EscapeThreshold > 0.0f ? AccumulatedStruggle / EscapeThreshold : 1.0f; }

	// 현재 누적 struggle(절대값)과 탈출 임계값(AI 붙잡는 힘). 디버깅/연출용.
	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	float GetAccumulatedStruggle() const { return AccumulatedStruggle; }

	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	float GetEscapeThreshold() const { return EscapeThreshold; }

	UPROPERTY(BlueprintAssignable, Category = "FT|Capture")
	FFTOnCaptureEscaped OnEscaped;

protected:
	// [플레이어의 탈출하는 힘 - 능동] 좌우 방향 전환 1회로 채워지는 struggle 양(절대값). 이 값을 GA_Grab의 EscapeThreshold만큼
	// 쌓으면 탈출. 예) 힘 1.0 vs 임계값 17.0 → 약 17번 전환이면 탈출. 값이 클수록 잘 빠져나온다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Capture", meta = (ClampMin = "0.0"))
	float StruggleGainPerFlip = 1.0f;

	// [플레이어의 탈출하는 힘 - 수동/자연증가] 연타와 무관하게 초당 저절로 차오르는 struggle. AI의 자연감소
	// (GA_Grab::EscapeDecayPerSecond)와 매 틱 힘싸움을 벌인다. 나중에 캐릭터 스탯(근력 등)으로 구동할 확장 지점.
	// 0이면 순수 연타 대결(자연증가 없음).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Capture", meta = (ClampMin = "0.0"))
	float StrugglePassiveGainPerSecond = 1.0f;

	// 방향 전환으로 인정할 최소 입력 크기(데드존). 작은 흔들림/노이즈 무시.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Capture", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StruggleInputDeadzone = 0.3f;

private:
	void SetStruggle(float NewStruggle);
	UAbilitySystemComponent* GetOwnerAbilitySystem() const;

	bool bCaptured = false;
	bool bEscaped = false;

	// 현재까지 쌓인 struggle(절대값, 0..EscapeThreshold). EscapeThreshold에 도달하면 탈출.
	float AccumulatedStruggle = 0.0f;

	// [AI의 붙잡는 힘] 탈출에 필요한 총 struggle. 캡처 시작 시 붙잡은 어빌리티(UFTGA_Grab)가 TryBeginCapture로 주입한다.
	float EscapeThreshold = 1.0f;

	// [AI의 탈출 저지력] 초당 누적 struggle을 되끌어내리는 양. GA_Grab이 TryBeginCapture로 주입(= EscapeDecayPerSecond).
	float EscapeDecayPerSecond = 0.0f;

	// 마지막으로 인정된 입력 방향 부호(-1/0/+1). 부호가 바뀌면 flip으로 게이지 상승.
	float LastStruggleSign = 0.0f;

	// 원복용: 붙잡히기 전 캡슐 콜리전 설정.
	TEnumAsByte<ECollisionEnabled::Type> SavedCollisionEnabled = ECollisionEnabled::QueryAndPhysics;

	// 원복용: 붙잡히기 전 컨트롤러 yaw 추종 설정. 캡처 중엔 꺼서 몸이 컨트롤 회전을 따라 돌지 않고
	// 부착된 캡처 포즈(경비 CapturePoint 회전)를 따르게 한다(시점은 스프링암으로 별도로 돈다).
	bool bSavedUseControllerRotationYaw = true;

	// 원복용: 붙잡은 경비(캡터)의 카메라 채널(ECC_Camera) 응답. 이송 중 플레이어 스프링암 프로브가
	// 경비 몸에 걸려 카메라를 몸속으로 당기지 않도록 잠시 Ignore로 바꾸고, 해제 시 원래 값으로 되돌린다.
	bool bCaptorCameraResponseSaved = false;
	TEnumAsByte<ECollisionResponse> SavedCaptorCapsuleCameraResponse = ECR_Block;
	TEnumAsByte<ECollisionResponse> SavedCaptorMeshCameraResponse = ECR_Block;

	TWeakObjectPtr<AActor> Captor;
};
