// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ProjectFT/Enum/FTSkillCheckResultType.h"
#include "FTInteractionComponent.generated.h"

class UFTChanneledInteractionComponent;

// 포커스된 상호작용 대상이 바뀔 때 호출된다(대상이 없어지면 nullptr). UI 프롬프트 갱신 등에 사용.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTOnFocusedInteractableChanged, AActor*, FocusedActor);

// 채널링(꾹 누르기) 대상이 바뀔 때. 시작 시 대상 액터, 종료 시 nullptr. UI 게이지 표시·숨김에 사용.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTOnActiveChannelChanged, AActor*, ChannelTarget);

// 활성 채널의 스킬체크 시작/종료를 플레이어 기준으로 중계한다(UI는 이 컴포넌트에만 한 번 바인딩하면 됨).
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFTOnActiveSkillCheckStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFTOnActiveSkillCheckEnded, EFTSkillCheckResultType, Result);

/**
 * 플레이어 시점에서 매 프레임 라인트레이스로 상호작용 가능한 대상을 찾아 포커스로 추적하고,
 * 상호작용 키 입력 시(TryInteract) 해당 대상에게 IFTInteractable::Interact를 전송한다.
 * 입력 자체는 받지 않는다 — 소유 캐릭터가 입력을 받아 TryInteract()를 호출한다.
 */
UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 상호작용 키를 누른 순간. 채널형 대상이면 채널링 시작, 아니면 즉시 상호작용.
	UFUNCTION(BlueprintCallable, Category = "FT|Interaction")
	void TryInteract();

	// 상호작용 키를 뗀 순간. 채널링 중이면 중단(진행도는 유지).
	UFUNCTION(BlueprintCallable, Category = "FT|Interaction")
	void StopInteract();

	// 스킬체크 입력. 채널링 중인 대상에게 전달한다.
	UFUNCTION(BlueprintCallable, Category = "FT|Interaction")
	void NotifySkillCheckInput();

	// 현재 포커스된 상호작용 대상(없으면 nullptr).
	UFUNCTION(BlueprintPure, Category = "FT|Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	// 포커스 대상이 바뀔 때 브로드캐스트(UI 등이 바인딩).
	UPROPERTY(BlueprintAssignable, Category = "FT|Interaction")
	FFTOnFocusedInteractableChanged OnFocusedInteractableChanged;

	//~ 채널형 상호작용/스킬체크 중계 — UI는 (바뀌는 대상이 아니라) 안 바뀌는 이 컴포넌트에만 한 번 바인딩한다.
	UPROPERTY(BlueprintAssignable, Category = "FT|Interaction")
	FFTOnActiveChannelChanged OnActiveChannelChanged;

	UPROPERTY(BlueprintAssignable, Category = "FT|Interaction")
	FFTOnActiveSkillCheckStarted OnSkillCheckStarted;

	UPROPERTY(BlueprintAssignable, Category = "FT|Interaction")
	FFTOnActiveSkillCheckEnded OnSkillCheckEnded;

	// 현재 채널링 중인 대상 액터(없으면 nullptr).
	UFUNCTION(BlueprintPure, Category = "FT|Interaction")
	AActor* GetActiveChannelActor() const;

	//~ 활성 채널 상태 폴링용(없으면 0/false). UI는 이 값들을 매 프레임 폴링(UMG 바인딩)한다.
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") bool IsChanneling() const;
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetChannelProgress() const;
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") bool IsSkillCheckActive() const;
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetSkillCheckCursor() const;
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetSkillCheckTarget() const;
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetSkillCheckSuccessHalfWidth() const;
	UFUNCTION(BlueprintPure, Category = "FT|Interaction") float GetSkillCheckGreatHalfWidth() const;

protected:
	// 시야에서 상호작용 가능한 대상을 라인트레이스로 찾는다(없으면 nullptr).
	AActor* TraceForInteractable() const;

	// 시점(카메라) 위치/전방을 구한다. 성공 시 true.
	bool GetViewPoint(FVector& OutLocation, FVector& OutDirection) const;

protected:
	// 트레이스 사거리(cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction", meta = (ClampMin = "0.0"))
	float InteractionDistance = 300.0f;

	// 트레이스에 사용할 콜리전 채널. 상호작용 대상이 이 채널을 Block 해야 감지된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction")
	TEnumAsByte<ECollisionChannel> InteractionChannel = ECC_Visibility;

	// 켜면 트레이스를 디버그 라인으로 그린다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Interaction")
	bool bDebugDrawTrace = false;

private:
	// 포커스 대상을 바꾸고, 변경됐으면 OnFocusedInteractableChanged를 브로드캐스트한다.
	void SetFocusedActor(AActor* NewFocusedActor);

	// 활성 채널을 바꾸며 스킬체크 델리게이트 bind/unbind를 한곳에서 처리한다(UI가 직접 관리할 필요 없게).
	void SetActiveChannel(UFTChanneledInteractionComponent* NewChannel);

	// 활성 채널의 스킬체크 이벤트를 플레이어 델리게이트로 중계한다.
	UFUNCTION()
	void HandleActiveSkillCheckStarted();
	UFUNCTION()
	void HandleActiveSkillCheckEnded(EFTSkillCheckResultType Result);

	// 현재 포커스된 대상(약참조 — 대상이 파괴돼도 안전).
	TWeakObjectPtr<AActor> FocusedActor;

	// 현재 채널링 중인 대상의 컴포넌트(없으면 invalid).
	TWeakObjectPtr<UFTChanneledInteractionComponent> ActiveChannel;
};
