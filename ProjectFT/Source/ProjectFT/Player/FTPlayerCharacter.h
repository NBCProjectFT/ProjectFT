// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "ProjectFT/Interface/FTInputInterface.h"
#include "FTPlayerCharacter.generated.h"

class UCameraComponent;
class UFTInteractionComponent;
class UAbilitySystemComponent;
class UFTAttributeSet;
class UFTItemDataAsset;
struct FOnAttributeChangeData;

UCLASS()
class PROJECTFT_API AFTPlayerCharacter : public ACharacter, public IFTDamageable, public IFTInputInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFTPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;

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
	virtual void HandleUseItemPressed() override;
	virtual void HandleSelectQuickSlot(int32 SlotIndex) override;
	//~ End IFTInputInterface

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface
    
    protected:
    	// Called when the game starts or when spawned
    	virtual void BeginPlay() override;
    
    	//~ Begin ACharacter
    	// 앉기/일어서기로 캡슐 높이가 바뀔 때 카메라가 순간이동하지 않도록 보간한다.
    	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
    	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
    	//~ End ACharacter
    
    	// 점프 대신 traversal(vault/hurdle/mantle)을 먼저 시도한다. 소비했으면 true. (확장 지점, 현재 false)
    	virtual bool TryStartTraversal();
    
    	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Camera", meta = (AllowPrivateAccess = "true"))
    	TObjectPtr<UCameraComponent> FirstPersonCamera;
    
    	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Interaction", meta = (AllowPrivateAccess = "true"))
    	TObjectPtr<UFTInteractionComponent> InteractionComponent;
    
    	// GAS: 능력/이펙트/속성의 허브. 속성값(체력/스태미나/이속 등)은 AttributeSet이 보유한다.
    	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|GAS", meta = (AllowPrivateAccess = "true"))
    	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
    
    	// 캐릭터의 서브오브젝트로 만들면 ASC가 InitializeComponent 시 자동 등록한다.
    	UPROPERTY()
        	TObjectPtr<UFTAttributeSet> AttributeSet;
        
        	// [Mock] 퀵슬롯 — 각 슬롯에 아이템 데이터 에셋(UFTItemDataAsset)을 지정한다. 사용 시 그 아이템의 UseData가 동작을 결정.
        	// 실제 인벤토리/장비가 붙기 전까지 '선택 키'로 고르고 '사용 키'로 사용하는 임시 슬롯이다.
        	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Item|Mock", meta = (AllowPrivateAccess = "true"))
        	TArray<TObjectPtr<UFTItemDataAsset>> MockQuickSlots;
        
        	// [Mock] 현재 선택된 퀵슬롯 인덱스. 추후 '손에 든 아이템'으로 대체된다.
        	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FT|Item|Mock", meta = (AllowPrivateAccess = "true"))
        	int32 SelectedQuickSlot = 0;
        
        	// 초당 스태미나 소진하는 양.
        	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0"))
        	float SprintStaminaCostPerSecond = 20.0f;
        
        	// 탈진 후 스프린트 재개에 필요한 최소 스태미나 비율(0~1). 프레임마다 빨라졌다 느려졌다 방지.
        	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
        	float SprintResumeStaminaFraction = 0.2f;
	
	// 스태미나 초당 회복량(사용 후 지연 뒤부터). 0이면 회복 안 함.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat|Regen", meta = (ClampMin = "0.0"))
	float StaminaRegenPerSecond = 15.0f;

	// 스태미나를 쓴 뒤 회복이 시작되기까지의 지연(초).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat|Regen", meta = (ClampMin = "0.0"))
	float StaminaRegenDelay = 1.0f;

	// 체력 초당 회복량. 0이면 회복 안 함(기본).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Stat|Regen", meta = (ClampMin = "0.0"))
	float HealthRegenPerSecond = 0.0f;

	// 앉기/일어서기 시 카메라가 이동하는 속도. 0이면 순간이동.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0"))
	float CrouchCameraInterpSpeed = 10.0f;

	// true면 점프 입력 시 traversal을 먼저 시도한다. (보류)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Traversal")
	bool bTryTraversalBeforeJump = false;

private:
	// 효과적 스프린트 상태(bIsSprinting)와 MoveSpeed 속성으로 MaxWalkSpeed/Crouched를 갱신한다.
	void ApplyMovementSpeed();

	// 스프린트 키/스태미나/크라우치 조건을 확인해 효과적 스프린트 상태를 갱신하고 스태미나 속성을 소모한다.
	void UpdateSprintState(float DeltaSeconds);

	// 스태미나/체력 회복(StatComponent에서 이전). 속성에 직접 적용한다.
	void UpdateStaminaRegen(float DeltaSeconds);
	
	// 이동속도 관련 속성(MoveSpeed/스프린트·앉기 배수)이 바뀌면 MaxWalkSpeed에 반영한다.
    	void OnSpeedAttributeChanged(const FOnAttributeChangeData& Data);
    
    	// 스턴 상태 태그(State.Debuff.Stun)가 붙고/풀릴 때 이동을 정지/복원한다.
    	void OnStunTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
    
    	// 체력이 0에 도달했을 때 호출(AttributeSet의 통지).
    	void HandleOutOfHealth();
    
    	// 현재 카메라 보정량(CrouchCameraOffsetZ)을 카메라 상대 위치에 반영한다.
    	void UpdateCrouchCameraOffset();
    
    	// 앉기/일어서기로 캡슐 중심이 실제로 이동한 경우에만 그만큼 카메라를 반대로 보정한다.
    	void ApplyCrouchCameraCompensation(float CameraOffsetDeltaZ);
    
    	// 스프린트 입력을 꾹 누르고 있는 동안 true(키 상태).
    	bool bSprintHeld = false;
    
    	// 실제로 스프린트가 적용 중인지(키 + 스태미나 + 비크라우치 조건 충족).
    	bool bIsSprinting = false;
    
    	// 스태미나 0으로 탈진한 상태. 일정 비율 회복 전까지 스프린트 재개를 막는다.
    	bool bSprintExhausted = false;
    
    	// 마지막 스태미나 사용 후 경과 시간(회복 지연 판정용).
    	float TimeSinceStaminaUse = 0.0f;
    
    	// 서 있을 때의 카메라 기본 상대 위치. 앉기 보간의 기준점.
    	FVector DefaultCameraRelativeLocation = FVector(0.0f, 0.0f, 70.0f);
    
    	// 캡슐 높이 변화로 생긴 카메라 Z 보정량. 매 프레임 0으로 보간된다.
    	float CrouchCameraOffsetZ = 0.0f;
    };