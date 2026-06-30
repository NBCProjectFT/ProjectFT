// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Character/FTCharacterBase.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Enum/FTWeaponStanceType.h"
#include "ProjectFT/Interface/FTInputInterface.h"
#include "FTPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFTInteractionComponent;
class UFTPlayerAttributeSet;
class UFTItemDataAsset;
class UFTGameplayAbility;
class AFTItemActor;
struct FOnAttributeChangeData;

// GAS 배선(ASC/공용 속성셋/IAbilitySystemInterface/사망 훅)은 AFTCharacterBase가 제공한다.
UCLASS()
class PROJECTFT_API AFTPlayerCharacter : public AFTCharacterBase, public IFTInputInterface
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
	virtual void HandleUseItemReleased() override;
	virtual void HandleSelectQuickSlot(int32 SlotIndex) override;
	virtual void HandleToggleInventoryPressed() override;
	//~ End IFTInputInterface

	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	bool IsInventoryOpen() const { return bInventoryOpen; }

	UFUNCTION(BlueprintCallable, Category = "FT|Inventory")
	void SetInventoryOpen(bool bNewInventoryOpen);

	UFUNCTION(BlueprintPure, Category = "FT|Item")
	const FFTInventoryItem& GetCurrentHeldInventoryItem() const { return CurrentHeldInventoryItem; }

	// 현재 손에 든 아이템의 스탠스(없으면 Unarmed). AnimBP의 Blend Poses by Enum 분기용.
	UFUNCTION(BlueprintPure, Category = "FT|Item")
	EFTWeaponStanceType GetHeldWeaponStance() const;

	// 채널형 상호작용(LootShelf 게이지 채우기 등)을 진행 중이면 true. AnimBP 모션 전환 분기용.
	// (꾹 누르고 있는 동안만 true — 키를 떼면 채널이 멈춰 false. 진행도/대상은 InteractionComponent에서 폴링.)
	UFUNCTION(BlueprintPure, Category = "FT|Interaction")
	bool IsChannelingInteraction() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 손에 든 아이템 액터는 메시에 어태치된 상태라 액터 파괴 시 자동 정리되지 않으므로 여기서 제거한다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~ Begin ACharacter
	// 앉기/일어서기로 캡슐 높이가 바뀔 때 카메라가 순간이동하지 않도록 보간한다.
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	//~ End ACharacter
    
	// 점프 대신 traversal(vault/hurdle/mantle)을 먼저 시도한다. 소비했으면 true. (확장 지점, 현재 false)
	virtual bool TryStartTraversal();
    
	// 3인칭 카메라 붐(스프링암): 캡슐에 붙어 컨트롤러 회전을 따라 돌고, 벽에 가리면 카메라를 앞으로 당겨 클리핑을 막는다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	// 붐 끝에 매달린 추적 카메라. 회전은 붐(bUsePawnControlRotation)이 담당한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTInteractionComponent> InteractionComponent;
    
	// 플레이어 전용 속성셋(스태미나/이동 배수/손재주). ASC·공용 AttributeSet은 베이스(AFTCharacterBase)가 보유하며,
	// 이 세트는 캐릭터 서브오브젝트라 베이스의 ASC에 자동 등록된다.
	UPROPERTY()
	TObjectPtr<UFTPlayerAttributeSet> PlayerAttributeSet;
        
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FT|Inventory", meta = (AllowPrivateAccess = "true"))
	bool bInventoryOpen = false;

	// 현재 플레이어가 손에 들고 있는 실질적인 아이템. 사용 입력은 이 아이템의 UseData를 기준으로 처리한다.
	// 직접 대입하지 말고 SetCurrentHeldInventoryItem()으로만 바꾼다(비주얼 액터 동기화를 위해).
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FT|Item", meta = (AllowPrivateAccess = "true"))
	FFTInventoryItem CurrentHeldInventoryItem;

	// 손에 든 아이템 비주얼로 스폰할 액터 클래스. 기본은 AFTItemActor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Item", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AFTItemActor> HeldItemActorClass;

	// 아이템 데이터에서 어태치 소켓을 못 찾았을 때 쓸 폴백 소켓 이름.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Item", meta = (AllowPrivateAccess = "true"))
	FName HeldItemFallbackSocketName = TEXT("MeleeHandGrip_R");
        
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
	// MoveSpeed×스프린트/앉기 배수로 MaxWalkSpeed/Crouched를 갱신한다(베이스의 기본 파생을 override).
	virtual void ApplyMovementSpeed() override;

	// 스프린트 키/스태미나/크라우치 조건을 확인해 효과적 스프린트 상태를 갱신하고 스태미나 속성을 소모한다.
	void UpdateSprintState(float DeltaSeconds);

	// 스태미나/체력 회복(StatComponent에서 이전). 속성에 직접 적용한다.
	void UpdateStaminaRegen(float DeltaSeconds);
	
	// 체력이 0에 도달했을 때 호출(베이스의 OnOutOfHealth 통지). 플레이어 사망 처리.
	virtual void HandleDeath() override;

	UFTInventoryComponent* GetInventoryComponent() const;

	bool EnsureUseAbilityGranted(TSubclassOf<UFTGameplayAbility> UseAbility);

	// 활성 중인 아이템 사용 어빌리티를 AssetTag(MatchTag) 기준으로 취소한다. 취소되면 효과/쿨다운은 적용되지 않는다.
	// CancelAbilities는 ActivationOwnedTags가 아니라 AssetTags를 매칭함에 주의 — 이동 시엔 .Channeled(조준형 투척은 유지),
	// 퀵슬롯 전환 시엔 부모 Ability.ItemUse로 종류 불문 취소.
	void CancelItemUseAbilities(FGameplayTag MatchTag);

	// 손에 든 아이템 변경의 단일 진입점. 같은 ItemId면 비주얼을 유지하고, 달라질 때만 액터를 교체한다.
	void SetCurrentHeldInventoryItem(const FFTInventoryItem& NewHeldItem);

	// CurrentHeldInventoryItem에 맞춰 손의 아이템 액터를 스폰/어태치하거나(없으면) 제거한다.
	void RefreshHeldItemActor();

	// 아이템 타입별 데이터 에셋의 AttachSocketName을 우선 사용하고, 없으면 폴백 소켓을 돌려준다.
	FName ResolveHeldItemAttachSocket(const UFTItemDataAsset* ItemData) const;
    
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
    
	// 서 있을 때의 카메라 붐 기본 상대 위치. 앉기 보간의 기준점.
	FVector DefaultBoomRelativeLocation = FVector(0.0f, 0.0f, 70.0f);
    
	// 캡슐 높이 변화로 생긴 카메라 Z 보정량. 매 프레임 0으로 보간된다.
	float CrouchCameraOffsetZ = 0.0f;

	// 현재 손에 어태치된 아이템 비주얼 액터. 들고 있지 않으면 nullptr.
	UPROPERTY(Transient)
	TObjectPtr<AFTItemActor> HeldItemActor = nullptr;
    };
