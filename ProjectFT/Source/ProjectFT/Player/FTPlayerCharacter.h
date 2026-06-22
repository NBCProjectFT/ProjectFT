// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "ProjectFT/Interface/FTInputInterface.h"
#include "FTPlayerCharacter.generated.h"

class UCameraComponent;
class UFTInteractionComponent;
class UFTPlayerStatComponent;
class UAbilitySystemComponent;

UCLASS()
class PROJECTFT_API AFTPlayerCharacter : public ACharacter, public IAbilitySystemInterface,
	public IFTDamageable, public IFTInputInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFTPlayerCharacter();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
	}

	//~ Begin IFTInputInterface
	// 컨트롤러가 정규화해 넘긴 입력으로 실제 이동/시점/점프 로직을 실행한다. (인풋 일괄 관리를 위해 컨트롤러에 구현)
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

	// 플레이어 스탯 컴포넌트 가져오기
	UFUNCTION(BlueprintPure, Category = "FT|Stat")
	UFTPlayerStatComponent* GetStatComponent() const { return StatComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//~ Begin ACharacter
	// 앉기/일어서기로 캡슐 높이가 바뀔 때 카메라가 순간이동하지 않도록 보간한다.
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	//~ End ACharacter

	// 점프 대신 traversal(vault/hurdle/mantle)을 먼저 시도한다. 소비했으면 true. (파쿠르 잔재긴 한데, 일단 놔둠.)
	// 실제 UFTTraversalComponent가 붙기 전까지 false를 반환하는 확장 지점이다(Docs/FPS_Traversal_Guide.md).
	virtual bool TryStartTraversal();
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTInteractionComponent> InteractionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Stat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTPlayerStatComponent> StatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	
	
	// 달리기 눌렀을 때 기본이속에 곱해지는 수치.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "1.0"))
	float SprintSpeedMultiplier = 1.5f;

	// 초당 스태미나 소진하는 양.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0"))
	float SprintStaminaCostPerSecond = 20.0f;

	// 스태미나를 전량 소진했을 때, 최소 이 값(0~1) 만큼은 다시 차올라야 달리기 가능해짐.(빨라졌다 느려졌다 프레임마다 반복하는 것 방지.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintResumeStaminaFraction = 0.2f;

	// 앉기 상태의 이동 속도. CharacterMovement의 MaxWalkSpeedCrouched에 반영한다. (이후 Mult형식으로 교체 예정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement")
	float CrouchSpeed = 300.0f;

	// 앉기/일어서기 시 카메라가 이동하는 속도. 높을수록 빨라지지만, 아예 0이면 순간이동 해버린다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0"))
	float CrouchCameraInterpSpeed = 10.0f;

	// true면 점프 입력 시 traversal을 먼저 시도한다. (폐기 예정. 인지하고 있으니 일단 보류. 사용할 필요는X)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Traversal")
	bool bTryTraversalBeforeJump = false;

private:
	// 효과적 스프린트 상태(bIsSprinting)와 StatComponent의 MoveSpeed로 MaxWalkSpeed를 갱신한다.
	void ApplyMovementSpeed();

	// 스프린트 키/스태미나/크라우치 조건 등을 확인해 효과적 스프린트 상태를 갱신하고 스태미나를 소모한다.
	void UpdateSprintState(float DeltaSeconds);

	// StatComponent의 MoveSpeed가 바뀌면 MaxWalkSpeed에 반영한다.
	UFUNCTION()
	void HandleMoveSpeedChanged(float NewMoveSpeed);

	// 현재 카메라 보정량(CrouchCameraOffsetZ)을 카메라 상대 위치에 반영한다.
	void UpdateCrouchCameraOffset();

	// 앉기/일어서기로 캡슐 중심이 실제로 이동한 경우(주로 지상)에만 그만큼 카메라를 반대로 보정한다.
	// 공중에선 중심이 고정이라 보정을 건너뛴다(보정하면 오히려 시점이 순간이동함).
	void ApplyCrouchCameraCompensation(float CameraOffsetDeltaZ);

	// 스프린트 입력을 꾹 누르고 있는 동안 true(키 상태).
	bool bSprintHeld = false;

	// 실제로 스프린트가 적용 중인지(키 + 스태미나 + 비크라우치 조건 충족).
	bool bIsSprinting = false;

	// 스태미나 0으로 탈진한 상태. 일정 비율 회복 전까지 스프린트 재개를 막는다.
	bool bSprintExhausted = false;

	// 서 있을 때의 카메라 기본 상대 위치. 앉기 보간의 기준점(BeginPlay에서 캐시, 생성자 값이 기본).
	FVector DefaultCameraRelativeLocation = FVector(0.0f, 0.0f, 70.0f);

	// 캡슐 높이 변화로 생긴 카메라 Z 보정량. 매 프레임 0으로 보간되어 시점이 점진적으로 목표에 도달한다.
	float CrouchCameraOffsetZ = 0.0f;
};
