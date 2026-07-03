// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "FTEscapeZoneActor.generated.h"

class UBoxComponent;
class USceneComponent;

/**
 * 월드에 배치하는 탈출 구역 액터.
 *
 * 지금 단계에서는 Box overlap으로 "플레이어가 탈출 구역에 들어왔다/나갔다"만 감지한다.
 * GameFlowSubsystem, UI, GameplayMessage 연결은 아직 하지 않고 아래 Hook 함수들에 연결 지점을 남긴다.
 *
 * 연결 예정 흐름:
 * - BeginEscapeOverlap  -> Request.Flow.StartEscape 메시지 전송
 * - EndEscapeOverlap    -> Request.Flow.CancelEscape 메시지 전송
 * - CompleteEscapeHold  -> Request.Flow.CompleteEscape 메시지 전송
 */

UCLASS()
class PROJECTFT_API AFTEscapeZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AFTEscapeZoneActor();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleEscapeZoneBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEscapeZoneEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void StartEscapeCountdown(AActor* EscapingActor);
	void CancelEscapeCountdown(AActor* EscapingActor);
	void CompleteEscapeCountdown();
	void TickEscapeCountdown();
	void BroadcastEscapeFlowRequest(const FGameplayTag& RequestTag, AActor* InstigatorActor) const;

	// 탈출 구역에 들어왔을 때 호출될 연결 지점. 이후 Flow StartEscape 요청을 여기서 보낸다.
	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Escape")
	void OnEscapeZoneEntered(AActor* EnteredActor);

	// 탈출 구역에서 나갔을 때 호출될 연결 지점. 이후 Flow CancelEscape 요청을 여기서 보낸다.
	UFUNCTION(BlueprintImplementableEvent, Category = "FT|Escape")
	void OnEscapeZoneExited(AActor* ExitedActor);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Escape", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Escape", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> EscapeCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Escape", meta = (ClampMin = "0.1"))
	float EscapeDuration = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "FT|Escape")
	TObjectPtr<AActor> CurrentEscapeActor = nullptr;

	FTimerHandle EscapeCountdownTimerHandle;
	float RemainingEscapeTime = 0.0f;
};
