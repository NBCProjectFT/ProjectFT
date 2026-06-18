// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "ProjectFT/Interface/FTInputInterface.h"
#include "FTPlayerCharacter.generated.h"

class UCameraComponent;
class UFTInteractionComponent;

UCLASS()
class PROJECTFT_API AFTPlayerCharacter : public ACharacter, public IFTDamageable, public IFTInputInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFTPlayerCharacter();

	// 매 프레임 호출. 현재는 앉기/일어서기 시점 보간에 사용한다.
	virtual void Tick(float DeltaSeconds) override;

	//~ Begin IFTInputInterface
	// 컨트롤러가 정규화해 넘긴 입력으로 실제 이동/시점/점프 로직을 실행한다.
	virtual void HandleMoveInput(const FVector2D& MoveValue) override;
	virtual void HandleLookInput(const FVector2D& LookValue) override;
	virtual void HandleJumpPressed() override;
	virtual void HandleJumpReleased() override;
	virtual void HandleSprintPressed() override;
	virtual void HandleSprintReleased() override;
	virtual void HandleCrouchPressed() override;
	virtual void HandleCrouchReleased() override;
	virtual void HandleInteractPressed() override;
	virtual void HandleInteractReleased() override;
	virtual void HandleSkillCheckPressed() override;
	//~ End IFTInputInterface

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//~ Begin ACharacter
	// 앉기/일어서기로 캡슐 높이가 바뀔 때 카메라가 순간이동하지 않도록 보정 오프셋을 건다.
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	//~ End ACharacter

	// 점프 대신 traversal(vault/hurdle/mantle)을 먼저 시도한다. 소비했으면 true.
	// 실제 UFTTraversalComponent가 붙기 전까지 false를 반환하는 확장 지점이다(Docs/FPS_Traversal_Guide.md).
	virtual bool TryStartTraversal();

protected:
	/** 1인칭 카메라. 눈높이에 붙고 컨트롤러 회전(pitch/yaw)을 직접 따른다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// 상호작용(시야 라인트레이스로 대상 감지 + 키 입력 시 대상에게 명령) 담당 컴포넌트.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTInteractionComponent> InteractionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement")
	float NormalSpeed = 600.0f;

	// 스프린트 입력을 꾹 누르는 동안 적용할 최대 이동 속도.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement")
	float SprintSpeed = 900.0f;

	// 크라우치(앉기) 상태의 최대 이동 속도. CharacterMovement의 MaxWalkSpeedCrouched에 반영한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement")
	float CrouchSpeed = 300.0f;

	// 앉기/일어서기 시 카메라가 목표 눈높이로 따라붙는 속도(FInterpTo). 클수록 빨리 도달, 0이면 즉시 이동.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0"))
	float CrouchCameraInterpSpeed = 10.0f;

	// true면 점프 입력 시 traversal을 먼저 시도한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Traversal")
	bool bTryTraversalBeforeJump = false;

private:
	// 스프린트 입력 유지 상태에 따라 CharacterMovement의 MaxWalkSpeed를 갱신한다.
	void ApplyMovementSpeed();

	// 현재 카메라 보정량(CrouchCameraOffsetZ)을 카메라 상대 위치에 반영한다.
	void UpdateCrouchCameraOffset();

	// 앉기/일어서기로 캡슐 중심이 실제로 이동한 경우(주로 지상)에만 그만큼 카메라를 반대로 보정한다.
	// 공중에선 중심이 고정이라 보정을 건너뛴다(보정하면 오히려 시점이 순간이동함).
	void ApplyCrouchCameraCompensation(float CameraOffsetDeltaZ);

	// 스프린트 입력을 꾹 누르고 있는 동안 true.
	bool bSprintHeld = false;

	// 서 있을 때의 카메라 기본 상대 위치. 앉기 보간의 기준점(BeginPlay에서 캐시, 생성자 값이 기본).
	FVector DefaultCameraRelativeLocation = FVector(0.0f, 0.0f, 70.0f);

	// 캡슐 높이 변화로 생긴 카메라 Z 보정량. 매 프레임 0으로 보간되어 시점이 점진적으로 목표에 도달한다.
	float CrouchCameraOffsetZ = 0.0f;
};
