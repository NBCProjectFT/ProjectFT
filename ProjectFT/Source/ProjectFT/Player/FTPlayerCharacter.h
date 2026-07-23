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
class UFTTraversalComponent;
class UFTCaptureEscapeComponent;
class UFTTailComponent;
class UFTCrosshairComponent;
class UFTPlayerAttributeSet;
class UFTItemDataAsset;
class UFTGameplayAbility;
class UFTUIManagerSubsystem;
class AFTItemActor;
struct FOnAttributeChangeData;

/** Player Blueprint defaults used to initialize the two AttributeSets owned by the player's ASC. */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTPlayerInitialAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes", meta = (ClampMin = "0.0"))
	float Health = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes", meta = (ClampMin = "0.0"))
	float MaxHealth = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes", meta = (ClampMin = "0.0"))
	float MoveSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes", meta = (ClampMin = "0.0"))
	float Stamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes", meta = (ClampMin = "0.0"))
	float Dexterity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes", meta = (ClampMin = "0.0"))
	float SprintSpeedMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes", meta = (ClampMin = "0.0"))
	float CrouchSpeedMultiplier = 0.5f;
};

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

	// 인벤토리 열림 상태는 UI 서브시스템이 단일 소스로 소유한다. 여기선 게임플레이/AnimBP가 읽기 편하도록 중계만 한다.
	UFUNCTION(BlueprintPure, Category = "FT|Inventory")
	bool IsInventoryOpen() const;

	UFUNCTION(BlueprintPure, Category = "FT|Item")
	const FFTInventoryItem& GetCurrentHeldInventoryItem() const { return CurrentHeldInventoryItem; }

	// 현재 손에 든 아이템의 스탠스(없으면 Unarmed). AnimBP의 Blend Poses by Enum 분기용.
	UFUNCTION(BlueprintPure, Category = "FT|Item")
	EFTWeaponStanceType GetHeldWeaponStance() const;

	// 채널형 상호작용(LootShelf 게이지 채우기 등)을 진행 중이면 true. AnimBP 모션 전환 분기용.
	// (한 번 누르면 유지되는 토글 — 이동/재입력/아이템 사용/행동불능/범위 이탈로 끊길 때 false. 진행도/대상은 InteractionComponent에서 폴링.)
	UFUNCTION(BlueprintPure, Category = "FT|Interaction")
	bool IsChannelingInteraction() const;

	// 경비에게 붙잡힌 상태(UFTGA_Grab). 이동/시점/아이템 입력이 막히고 좌우 연타 탈출만 허용된다.
	UFUNCTION(BlueprintPure, Category = "FT|Capture")
	bool IsCaptured() const;

	// 현재 ASC 속성으로 계산한 최대 이동 속도. AnimBP가 블렌드 좌표와 재생 속도를
	// 같은 기준으로 정규화할 수 있도록 원본 애니메이션 제작 속도와 분리해 제공한다.
	UFUNCTION(BlueprintPure, Category = "FT|Movement")
	float GetSprintMovementSpeed() const;

	UFUNCTION(BlueprintPure, Category = "FT|Movement")
	float GetCrouchMovementSpeed() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 손에 든 아이템 액터는 메시에 어태치된 상태라 액터 파괴 시 자동 정리되지 않으므로 여기서 제거한다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~ Begin ACharacter
	// 앉기/일어서기로 캡슐 높이가 바뀔 때 카메라가 순간이동하지 않도록 보간한다.
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// 스태미나가 JumpStaminaCost에 못 미치면 점프를 불허한다. 입력 시점이 아니라 여기서 막는 이유는
	// 엔진이 점프 성립 여부를 이 함수로 판정하기 때문 — 키를 누른 채 착지하는 연속 점프까지 한 곳에서 걸러진다.
	virtual bool CanJumpInternal_Implementation() const override;

	// 점프가 실제로 성립한 순간에만 1회 호출된다(가변 높이 유지 프레임엔 재호출 안 됨). 스태미나 소모 지점.
	virtual void OnJumped_Implementation() override;
	//~ End ACharacter
    
	// 점프 대신 traversal(vault/hurdle/mantle)을 먼저 시도한다. 소비했으면 true. (확장 지점, 현재 false)
	virtual bool TryStartTraversal();
    
	// 3인칭 카메라 붐(스프링암): 캡슐에 붙어 컨트롤러 회전을 따라 돌고, 벽에 가리면 카메라를 앞으로 당겨 클리핑을 막는다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	// 붐 끝에 매달린 추적 카메라. 회전은 붐(bUsePawnControlRotation)이 담당한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// Tail Socket에 붙는 별도 꼬리 컴포넌트. 메시와 절차적 흔들림은 컴포넌트가 소유한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Tail", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTTailComponent> TailComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Crosshair", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTCrosshairComponent> CrosshairComponent;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTInteractionComponent> InteractionComponent;

	// 붙잡힘(경비 잡기) 상태·좌우연타 탈출 게이지를 소유한다. UFTGA_Grab이 BeginCapture/EndCapture로 구동한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Capture", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTCaptureEscapeComponent> CaptureEscapeComponent;

	// 트레이스 기반 파쿠르(Vault/Hurdle/Mantle). 점프 입력 시 TryStartTraversal에서 사용한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Traversal", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTTraversalComponent> TraversalComponent;
    
	// 플레이어 전용 속성셋(스태미나/이동 배수/손재주). ASC·공용 AttributeSet은 베이스(AFTCharacterBase)가 보유하며,
	// 이 세트는 캐릭터 서브오브젝트라 베이스의 ASC에 자동 등록된다.
	UPROPERTY()
	TObjectPtr<UFTPlayerAttributeSet> PlayerAttributeSet;

	// 플레이어 ASC의 공용/전용 AttributeSet에 BeginPlay 시 적용할 초기 스탯.
	// BP 클래스 기본값 또는 레벨 인스턴스에서 조정할 수 있으며, 기존 하드코딩 기본값과 동일하게 시작한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|GAS|Initial Attributes",
		meta = (ShowOnlyInnerProperties, AllowPrivateAccess = "true"))
	FFTPlayerInitialAttributes InitialAttributes;
        
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

	// 점프 1회당 소모하는 스태미나. 이만큼 없으면 점프 자체가 막힌다. 0이면 점프는 스태미나를 쓰지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement", meta = (ClampMin = "0.0"))
	float JumpStaminaCost = 15.0f;
        
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

	// 서 있을 때 캡슐 높이에 대한 앉은 캡슐 높이의 비율.
	// BeginPlay에서 BP/인스턴스에 적용된 실제 캡슐 크기를 기준으로 CrouchedHalfHeight를 계산한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Movement",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CrouchCapsuleHeightRatio = 0.65f;

	// true면 점프 입력 시 traversal(vault/hurdle/mantle)을 먼저 시도하고, 장애물이 없으면 일반 점프한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Traversal")
	bool bTryTraversalBeforeJump = true;

	// 발버둥 탈출 flip으로 인정할 최소 이동 입력 크기(데드존). 작은 흔들림/노이즈 무시.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Escape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StruggleInputDeadzone = 0.3f;

private:
	// 에디터에 설정된 InitialAttributes를 ASC의 base value로 반영한 뒤 파생 이동속도를 갱신한다.
	void ApplyInitialAbilitySystemAttributes();

	// MoveSpeed×스프린트/앉기 배수로 MaxWalkSpeed/Crouched를 갱신한다(베이스의 기본 파생을 override).
	virtual void ApplyMovementSpeed() override;

	// 스프린트 키/스태미나/크라우치 조건을 확인해 효과적 스프린트 상태를 갱신하고 스태미나 속성을 소모한다.
	void UpdateSprintState(float DeltaSeconds);

	// 점프 1회 비용을 낼 스태미나가 있는지. 비용이 0이거나 ASC/속성셋이 아직 없으면 점프를 막지 않는다.
	bool HasEnoughStaminaForJump() const;

	// 스태미나/체력 회복(StatComponent에서 이전). 속성에 직접 적용한다.
	void UpdateStaminaRegen(float DeltaSeconds);

	// 탈출 가능 상태에서 이동 입력의 좌우 전환(flip)을 감지해 Event.Struggle을 1발 발행한다.
	// 캡처 컴포넌트 또는 GE 기반 탈출 GA가 이 이벤트를 받아 게이지를 올린다.
	void SendStruggleOnFlip(float MoveAxisX);

	// 발버둥 flip 판정용: 마지막으로 인정된 이동 X축 방향 부호(-1/0/+1).
	float StruggleLastSign = 0.0f;

	// 플레이어 사망 후처리(베이스 HandleDeath가 태그/능력취소/이동정지를 끝낸 뒤 호출). 입력 차단까지 담당한다.
	virtual void OnDeath() override;

	// 행동불능 시작/해제 시 확장 훅. 플레이어는 여기서 진행 중이던 채널형 상호작용을 끊는다
	// (베이스의 어빌리티 취소·이동 봉쇄로는 채널에 닿지 않기 때문 — 구현부 주석 참조).
	virtual void OnImmobilizedStateChanged(bool bImmobilized) override;

	UFTInventoryComponent* GetInventoryComponent() const;

	// 게임 인스턴스에서 UI 매니저 서브시스템을 가져온다(인벤토리 열림 판정/토글 위임용).
	UFTUIManagerSubsystem* GetUIManager() const;

	bool EnsureUseAbilityGranted(TSubclassOf<UFTGameplayAbility> UseAbility);

	// 활성 중인 아이템 사용 어빌리티를 AssetTag(MatchTag) 기준으로 취소한다. 취소되면 효과/쿨다운은 적용되지 않는다.
	// CancelAbilities는 ActivationOwnedTags가 아니라 AssetTags를 매칭함에 주의 — 이동 시엔 .Channeled(조준형 투척은 유지),
	// 퀵슬롯 전환 시엔 부모 Ability.ItemUse로 종류 불문 취소.
	void CancelItemUseAbilities(FGameplayTag MatchTag);

	// 손에 든 아이템 변경의 단일 진입점. 같은 ItemId면 비주얼을 유지하고, 달라질 때만 액터를 교체한다.
	// bPlaySound=false면 꺼내기/집어넣기 효과음을 생략한다 — 마지막 개수를 다 써서 손에서 사라지는 경우처럼
	// 플레이어가 직접 넣고 뺀 게 아닌 해제에 쓴다(소모는 사용음의 몫이지 장착음의 몫이 아니다).
	void SetCurrentHeldInventoryItem(const FFTInventoryItem& NewHeldItem, bool bPlaySound = true);

	// 꺼내기/집어넣기 효과음을 아이템 데이터에서 골라 사용자에게 붙여 재생한다. 데이터나 사운드가 없으면 no-op.
	void PlayHeldItemSound(const UFTItemDataAsset* ItemData, bool bEquipped) const;

	// CurrentHeldInventoryItem에 맞춰 손의 아이템 액터를 스폰/어태치하거나(없으면) 제거한다.
	void RefreshHeldItemActor();

	UFUNCTION()
	void OnInventoryChangedCallback();

	// 채널형 상호작용 시작(대상 액터)/종료(nullptr) 시 호출된다. 채널 중엔 몸을 그 자리에 고정하고 시점만 돌게 한다.
	// 채널이 어떤 경로로 끝나든(완료·재입력·이동·범위 이탈·대상 파괴) 이 델리게이트가 종료를 알리므로 원복이 새지 않는다.
	UFUNCTION()
	void HandleActiveChannelChanged(AActor* ChannelTarget);

	// 아이템 타입별 데이터 에셋의 AttachSocketName을 우선 사용하고, 없으면 폴백 소켓을 돌려준다.
	FName ResolveHeldItemAttachSocket(const UFTItemDataAsset* ItemData) const;
    
	// 현재 카메라 보정량(CrouchCameraOffsetZ)을 카메라 상대 위치에 반영한다.
	void UpdateCrouchCameraOffset();
    
	// 앉기/일어서기로 캡슐 중심이 실제로 이동한 경우에만 그만큼 카메라를 반대로 보정한다.
	void ApplyCrouchCameraCompensation(float CameraOffsetDeltaZ);

	// BP/인스턴스의 초기 캡슐 크기를 저장하고 CrouchCapsuleHeightRatio로 앉은 높이를 설정한다.
	void InitializeCrouchCapsuleSize();
    
	// 스프린트 입력을 꾹 누르고 있는 동안 true(키 상태).
	bool bSprintHeld = false;
    
	// 실제로 스프린트가 적용 중인지(키 + 스태미나 + 비크라우치 조건 충족).
	// AnimBP가 이동 상태를 분기할 수 있도록 읽기 전용으로 노출한다(쓰기는 UpdateSprintState 단독).
	UPROPERTY(BlueprintReadOnly, Category = "FT|Movement", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;
    
	// 스태미나 0으로 탈진한 상태. 일정 비율 회복 전까지 스프린트 재개를 막는다.
	bool bSprintExhausted = false;
    
	// 마지막 스태미나 사용 후 경과 시간(회복 지연 판정용).
	float TimeSinceStaminaUse = 0.0f;
    
	// 서 있을 때의 카메라 붐 기본 상대 위치. 앉기 보간의 기준점.
	FVector DefaultBoomRelativeLocation = FVector(0.0f, 0.0f, 70.0f);
    
	// 캡슐 높이 변화로 생긴 카메라 Z 보정량. 매 프레임 0으로 보간된다.
	float CrouchCameraOffsetZ = 0.0f;

	// BeginPlay 시점의 실제 캡슐 크기. BP가 네이티브 생성자 기본값을 덮어쓴 결과까지 반영한다.
	float InitialCapsuleRadius = 0.0f;
	float InitialCapsuleHalfHeight = 0.0f;

	// 현재 손에 어태치된 아이템 비주얼 액터. 들고 있지 않으면 nullptr.
	UPROPERTY(Transient)
	TObjectPtr<AFTItemActor> HeldItemActor = nullptr;

	// 아이템을 사용하는 공용 헬퍼 함수 (마우스 클릭 및 퀵슬롯 즉발 사용 공유)
	void UseInventoryItem(const FFTInventoryItem& InventoryItem);
    };
