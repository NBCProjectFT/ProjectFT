// Fill out your copyright notice in the Description page of Project Settings.

#include "FTPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "ProjectFT/AbilitySystem/Abilities/FTGA_UseItem.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"

// Sets default values
AFTPlayerCharacter::AFTPlayerCharacter()
{
	// 플레이어 캐릭터는 매 프레임 Tick한다(앉기 시점 보간, 스프린트/회복 등).
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// 시점(컨트롤러 회전)에 본체 yaw를 맞춘다(FPS 프로토타입).
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		// 초기값(CDO/프리뷰용). 런타임엔 BeginPlay의 ApplyMovementSpeed가 MoveSpeed 속성으로 덮어쓴다.
		Movement->MaxWalkSpeed = 600.0f;
		Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
		Movement->MaxWalkSpeedCrouched = 300.0f;
	}

	// 1인칭 카메라: 눈높이에 붙이고 컨트롤러 회전(pitch/yaw)을 직접 따르게 한다.
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetOwnerNoSee(true);
	}

	// 상호작용 컴포넌트.
	InteractionComponent = CreateDefaultSubobject<UFTInteractionComponent>(TEXT("InteractionComponent"));

	// GAS: 능력시스템 컴포넌트 + 속성셋. 속성셋은 캐릭터 서브오브젝트라 ASC가 자동 등록한다.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UFTAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AFTPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void AFTPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// 싱글: 소유자=아바타=this. (InitializeComponent가 한 번 호출하지만 명시적으로 한 번 더 — 안전.)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// MoveSpeed 속성이 바뀌면(버프/디버프) MaxWalkSpeed에 즉시 반영.
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &AFTPlayerCharacter::OnMoveSpeedAttributeChanged);

		// [Mock] 퀵슬롯의 아이템 사용 어빌리티들을 부여한다(인덱스=슬롯). 실제 인벤토리/장비가 붙으면 교체.
		for (const TSubclassOf<UFTGA_UseItem>& AbilityClass : MockQuickSlots)
		{
			if (AbilityClass)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass));
			}
		}
	}

	if (AttributeSet)
	{
		AttributeSet->OnOutOfHealth.AddUObject(this, &AFTPlayerCharacter::HandleOutOfHealth);
	}

	// 초기 속성값으로 서기/스프린트/앉기 속도를 일괄 반영한다.
	ApplyMovementSpeed();

	// 서 있을 때의 카메라 상대 위치를 앉기 보간의 기준점으로 캐시한다.
	if (FirstPersonCamera)
	{
		DefaultCameraRelativeLocation = FirstPersonCamera->GetRelativeLocation();
	}
}

void AFTPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 앉기 카메라 보정량을 0으로 부드럽게 보간.
	if (!FMath::IsNearlyZero(CrouchCameraOffsetZ))
	{
		CrouchCameraOffsetZ = FMath::FInterpTo(CrouchCameraOffsetZ, 0.0f, DeltaSeconds, CrouchCameraInterpSpeed);
		UpdateCrouchCameraOffset();
	}

	// 스프린트(스태미나 소모) → 회복 순서로 처리(소모가 회복 지연 타이머를 리셋).
	UpdateSprintState(DeltaSeconds);
	UpdateStaminaRegen(DeltaSeconds);
}

void AFTPlayerCharacter::HandleMoveInput(const FVector2D& MoveValue)
{
	if (!Controller)
	{
		return;
	}

	// 아이템 시전 중 이동하면 시전을 취소한다(채널링 중단). 취소 시 효과/쿨다운은 적용되지 않는다.
	if (AbilitySystemComponent && !MoveValue.IsNearlyZero()
		&& AbilitySystemComponent->HasMatchingGameplayTag(TAG_FT_State_UsingItem))
	{
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(TAG_FT_State_UsingItem);
		AbilitySystemComponent->CancelAbilities(&CancelTags);
	}

	// UE 표준 컨벤션: MoveValue.Y = 전방, MoveValue.X = 우측. 축 구성은 IMC에서 맞춘다.
	const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix YawMatrix(YawRotation);

	if (!FMath::IsNearlyZero(MoveValue.Y))
	{
		AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), MoveValue.Y);
	}

	if (!FMath::IsNearlyZero(MoveValue.X))
	{
		AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), MoveValue.X);
	}
}

void AFTPlayerCharacter::HandleLookInput(const FVector2D& LookValue)
{
	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);
}

void AFTPlayerCharacter::HandleJumpPressed()
{
	if (bTryTraversalBeforeJump && TryStartTraversal())
	{
		return;
	}

	Jump();
}

void AFTPlayerCharacter::HandleJumpReleased()
{
	StopJumping();
}

void AFTPlayerCharacter::HandleSprintPressed()
{
	// 키 상태만 기록. 조건 확인과 속도 적용은 Tick의 UpdateSprintState가 처리.
	bSprintHeld = true;
}

void AFTPlayerCharacter::HandleSprintReleased()
{
	bSprintHeld = false;
}

void AFTPlayerCharacter::HandleCrouchPressed()
{
	Crouch();
}

void AFTPlayerCharacter::HandleCrouchReleased()
{
	UnCrouch();
}

void AFTPlayerCharacter::HandleInteractPressed()
{
	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AFTPlayerCharacter::HandleInteractReleased()
{
	if (InteractionComponent)
	{
		InteractionComponent->StopInteract();
	}
}

void AFTPlayerCharacter::HandleSkillCheckPressed()
{
	if (InteractionComponent)
	{
		InteractionComponent->NotifySkillCheckInput();
	}
}

void AFTPlayerCharacter::HandleUseItemPressed()
{
	// [Mock] 현재 선택된 퀵슬롯의 아이템 사용 어빌리티를 활성화한다. 추후 인벤토리/장비의 '손에 든 아이템'으로 교체.
	if (!AbilitySystemComponent || !MockQuickSlots.IsValidIndex(SelectedQuickSlot))
	{
		return;
	}

	const TSubclassOf<UFTGA_UseItem> AbilityClass = MockQuickSlots[SelectedQuickSlot];
	if (!AbilityClass)
	{
		return;
	}

	// 쿨다운/시전시간/효과 적용은 어빌리티가 담당한다(쿨다운 중이면 활성화가 거부된다).
	AbilitySystemComponent->TryActivateAbilityByClass(AbilityClass);
}

void AFTPlayerCharacter::HandleSelectQuickSlot(int32 SlotIndex)
{
	// [Mock] 선택만 바꾼다. 인덱스 기반이라 슬롯 수가 늘어도(키만 추가) 이 로직은 그대로다.
	if (MockQuickSlots.IsValidIndex(SlotIndex))
	{
		SelectedQuickSlot = SlotIndex;
		UE_LOG(LogFTPlayer, Verbose, TEXT("QuickSlot %d selected on '%s'."), SlotIndex, *GetName());
	}
}

void AFTPlayerCharacter::ApplyMovementSpeed()
{
	if (!AttributeSet)
	{
		return;
	}

	// 서기/스프린트/앉기 속도를 모두 MoveSpeed 속성(버프 포함 최종값)에서 파생한다.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		const float BaseSpeed = AttributeSet->GetMoveSpeed();
		Movement->MaxWalkSpeed = bIsSprinting ? BaseSpeed * AttributeSet->GetSprintSpeedMultiplier() : BaseSpeed;
		Movement->MaxWalkSpeedCrouched = BaseSpeed * AttributeSet->GetCrouchSpeedMultiplier();
	}
}

void AFTPlayerCharacter::UpdateSprintState(float DeltaSeconds)
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	const float Stamina = AttributeSet->GetStamina();
	const float MaxStamina = AttributeSet->GetMaxStamina();

	// 탈진 해제: 스태미나가 최대치의 SprintResumeStaminaFraction 이상으로 회복되면 다시 스프린트 가능.
	if (bSprintExhausted && Stamina >= MaxStamina * SprintResumeStaminaFraction)
	{
		bSprintExhausted = false;
	}

	bool bSprinting = bSprintHeld && !bIsCrouched && !bSprintExhausted && Stamina > 0.0f;

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const bool bMovingOnGround = Movement && Movement->IsMovingOnGround() && GetVelocity().SizeSquared() > FMath::Square(10.0f);
	if (bSprinting && bMovingOnGround)
	{
		// 스태미나 속성을 직접 감소(클램프는 PreAttributeChange가 처리). 소모 시 회복 지연 타이머 리셋.
		AbilitySystemComponent->ApplyModToAttribute(UFTAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -SprintStaminaCostPerSecond * DeltaSeconds);
		TimeSinceStaminaUse = 0.0f;

		if (AttributeSet->GetStamina() <= 0.0f)
		{
			bSprintExhausted = true;
			bSprinting = false;
		}
	}

	if (bSprinting != bIsSprinting)
	{
		bIsSprinting = bSprinting;
		ApplyMovementSpeed();
	}
}

void AFTPlayerCharacter::UpdateStaminaRegen(float DeltaSeconds)
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	TimeSinceStaminaUse += DeltaSeconds;

	// 스태미나 회복(마지막 사용 후 지연이 지난 다음부터).
	if (StaminaRegenPerSecond > 0.0f && TimeSinceStaminaUse >= StaminaRegenDelay
		&& AttributeSet->GetStamina() < AttributeSet->GetMaxStamina())
	{
		AbilitySystemComponent->ApplyModToAttribute(UFTAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, StaminaRegenPerSecond * DeltaSeconds);
	}

	// 체력 회복(기본 0이라 보통 비활성).
	if (HealthRegenPerSecond > 0.0f && AttributeSet->GetHealth() > 0.0f
		&& AttributeSet->GetHealth() < AttributeSet->GetMaxHealth())
	{
		AbilitySystemComponent->ApplyModToAttribute(UFTAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, HealthRegenPerSecond * DeltaSeconds);
	}
}

void AFTPlayerCharacter::OnMoveSpeedAttributeChanged(const FOnAttributeChangeData& Data)
{
	// MoveSpeed 속성 변경(버프/디버프 등)을 즉시 MaxWalkSpeed에 반영한다.
	ApplyMovementSpeed();
}

void AFTPlayerCharacter::HandleOutOfHealth()
{
	UE_LOG(LogFTPlayer, Log, TEXT("'%s' died (health depleted)."), *GetNameSafe(this));
	// 사망 후처리(레벨 전환 등)는 GameFlow 연동으로 — 이번 스코프 밖.
}

void AFTPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ApplyCrouchCameraCompensation(+HalfHeightAdjust);
}

void AFTPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ApplyCrouchCameraCompensation(-HalfHeightAdjust);
}

void AFTPlayerCharacter::ApplyCrouchCameraCompensation(float CameraOffsetDeltaZ)
{
	// 엔진이 base 고정 시(주로 지상)에만 캡슐 중심을 옮기므로, 그 경우에만 반대로 보정한다.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || !Movement->bCrouchMaintainsBaseLocation)
	{
		return;
	}

	CrouchCameraOffsetZ += CameraOffsetDeltaZ;
	UpdateCrouchCameraOffset();
}

void AFTPlayerCharacter::UpdateCrouchCameraOffset()
{
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetRelativeLocation(DefaultCameraRelativeLocation + FVector(0.0f, 0.0f, CrouchCameraOffsetZ));
	}
}

bool AFTPlayerCharacter::TryStartTraversal()
{
	return false;
}
