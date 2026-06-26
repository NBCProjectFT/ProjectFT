// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FTInputInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UFTInputInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 정규화된 이동/시점/점프 입력을 받아 실제 로직을 실행하는 대상(주로 Character)이 구현한다.
 * 컨트롤러는 이 인터페이스로만 입력을 전달하므로 구체 캐릭터 타입에 의존하지 않는다.
 */
class PROJECTFT_API IFTInputInterface
{
	GENERATED_BODY()

public:
	// MoveValue: Move 액션(Axis2D)의 IMC 출력 원본. UE 표준 컨벤션 기준 X = 우측(Right), Y = 전방(Forward).
	virtual void HandleMoveInput(const FVector2D& MoveValue) = 0;

	// LookValue: Look 액션(Axis2D)의 IMC 출력 원본. X = Yaw, Y = Pitch. 반전이 필요하면 IMC Modifier로 처리.
	virtual void HandleLookInput(const FVector2D& LookValue) = 0;

	// 점프 입력이 눌린 순간.
	virtual void HandleJumpPressed() = 0;

	// 점프 입력에서 손을 뗀 순간.
	virtual void HandleJumpReleased() = 0;

	// 스프린트 입력이 눌린 순간(꾹 누르는 동안 가속).
	virtual void HandleSprintPressed() = 0;

	// 스프린트 입력에서 손을 뗀 순간(기본 속도로 복귀).
	virtual void HandleSprintReleased() = 0;

	// 크라우치 입력이 눌린 순간(꾹 누르는 동안 앉기).
	virtual void HandleCrouchPressed() = 0;

	// 크라우치 입력에서 손을 뗀 순간(일어서기).
	virtual void HandleCrouchReleased() = 0;

	// 상호작용 입력이 눌린 순간.
	virtual void HandleInteractPressed() = 0;

	// 상호작용 입력에서 손을 뗀 순간(채널형 상호작용 중단).
	virtual void HandleInteractReleased() = 0;

	// 스킬체크 입력이 눌린 순간.
	virtual void HandleSkillCheckPressed() = 0;

	// 아이템 사용 입력이 눌린 순간(현재 선택된 퀵슬롯을 사용).
	virtual void HandleUseItemPressed() = 0;

	// 퀵슬롯 선택 입력(SlotIndex번 슬롯을 현재 선택으로 둔다). 실제 사용은 HandleUseItemPressed가 수행한다.
	virtual void HandleSelectQuickSlot(int32 SlotIndex) = 0;
	
	// 인벤토리 토글
	virtual void HandleToggleInventoryPressed() = 0;
};
