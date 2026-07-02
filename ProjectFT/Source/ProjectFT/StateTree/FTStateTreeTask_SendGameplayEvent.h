// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "FTStateTreeTask_SendGameplayEvent.generated.h"

class AActor;

USTRUCT()
struct FFTSendGameplayEventTaskInstanceData
{
	GENERATED_BODY()

	// 이벤트를 받을 액터(= 어빌리티 소유자). AI StateTree에선 보통 Context의 Actor(경비 폰)에 바인딩한다.
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> EventOwner = nullptr;

	// 페이로드 Target(잡을 대상 = 플레이어). 블랙보드/파라미터로 바인딩(비우면 어빌리티가 알아서 플레이어0을 잡음).
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<AActor> PayloadTarget = nullptr;

	// 보낼 GameplayEvent 태그. 기본은 잡기(Event.Grab).
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag EventTag = TAG_FT_Event_Grab;

	// 이 태그가 EventOwner에 붙어있는 동안 태스크를 Running으로 유지한다(= 발동된 어빌리티가 살아있는 동안).
	// 비우면 이벤트만 보내고 즉시 Succeeded. 기본은 잡기 진행 태그(State.Grabbing).
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag RunningWhileTag = TAG_FT_State_Grabbing;
};

/**
 * EventOwner의 ASC로 GameplayEvent를 보내는 StateTree 태스크.
 * 예) 경비 StateTree의 잡기 상태에서 EventOwner=경비, EventTag=Event.Grab, PayloadTarget=플레이어로 두면 UFTGA_Grab이 발동한다.
 * RunningWhileTag가 지정되면 그 태그가 EventOwner에 붙어있는 동안 상태를 유지(Running)하다가 사라지면 Succeeded —
 * 즉 잡기 어빌리티가 끝날 때까지 StateTree의 잡기 상태가 정확히 유지된다. (컨트롤러가 아니라 StateTree가 그랩을 지시한다.)
 */
USTRUCT(meta = (DisplayName = "FT Send Gameplay Event", Category = "FT|Ability"))
struct FFTStateTreeTask_SendGameplayEvent : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FFTSendGameplayEventTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
