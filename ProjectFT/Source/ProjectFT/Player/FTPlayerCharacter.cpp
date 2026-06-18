// Fill out your copyright notice in the Description page of Project Settings.

#include "FTPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Components/FTPlayerStatComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"

// Sets default values
AFTPlayerCharacter::AFTPlayerCharacter()
{
	// 플레이어 캐릭터는 매 프레임 Tick한다(앉기 시점 보간 등). 단일 액터라 비용은 무시할 수준이고,
	// 액터 Tick을 동적으로 켜고 끄지 않아 나중에 Tick에 다른 기능을 넣어도 서로 간섭하지 않는다.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// 시점(컨트롤러 회전)에 본체 yaw를 맞춘다(FPS 프로토타입). 카메라는 스프링 암이 따라간다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		// 초기값(CDO/프리뷰용). 런타임엔 BeginPlay의 ApplyMovementSpeed가 StatComponent의 MoveSpeed로 덮어쓴다.
		Movement->MaxWalkSpeed = 600.0f;

		// 크라우치(앉기)를 허용하고 앉은 상태의 이동 속도를 설정한다.
		Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
		Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	}

	// 1인칭 카메라: 눈높이에 붙이고 컨트롤러 회전(pitch/yaw)을 직접 따르게 한다.
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f)); // 캡슐 중심 기준 눈높이
	FirstPersonCamera->bUsePawnControlRotation = true;

	// 1인칭이라 자기 본체 메시는 시야에서 가린다(머리 안쪽이 보이는 문제 방지). 다른 시점/그림자에는 영향 없음.
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetOwnerNoSee(true);
	}

	// 상호작용 컴포넌트: 시야 라인트레이스로 대상 감지, 키 입력 시 상호작용 명령 전송.
	InteractionComponent = CreateDefaultSubobject<UFTInteractionComponent>(TEXT("InteractionComponent"));

	// 플레이어 스탯(체력/스태미나/이동속도/손재주) 컴포넌트.
	StatComponent = CreateDefaultSubobject<UFTPlayerStatComponent>(TEXT("StatComponent"));
}

// Called when the game starts or when spawned
void AFTPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 디자이너가 BP/인스턴스에서 조정한 이동 속도 값을 런타임에 반영한다.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeedCrouched = CrouchSpeed;
	}

	// MoveSpeed 스탯이 바뀌면 MaxWalkSpeed에 반영되도록 바인딩(같은 액터라 언바인드 불필요).
	if (StatComponent)
	{
		StatComponent->OnMoveSpeedChanged.AddDynamic(this, &AFTPlayerCharacter::HandleMoveSpeedChanged);
	}

	ApplyMovementSpeed();

	// 서 있을 때의 카메라 상대 위치를 앉기 보간의 기준점으로 캐시한다(BP/인스턴스 오버라이드 반영).
	if (FirstPersonCamera)
	{
		DefaultCameraRelativeLocation = FirstPersonCamera->GetRelativeLocation();
	}
}

void AFTPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 캡슐 높이 변화로 생긴 카메라 보정량을 0으로 부드럽게 줄여, 시점이 점진적으로 목표 눈높이에 도달하게 한다.
	// 보정할 게 없으면 바로 빠져나간다(액터 Tick 자체는 끄지 않는다).
	if (!FMath::IsNearlyZero(CrouchCameraOffsetZ))
	{
		CrouchCameraOffsetZ = FMath::FInterpTo(CrouchCameraOffsetZ, 0.0f, DeltaSeconds, CrouchCameraInterpSpeed);
		UpdateCrouchCameraOffset();
	}

	// 스프린트 효과적 상태 갱신 + 스태미나 소모.
	UpdateSprintState(DeltaSeconds);
}

void AFTPlayerCharacter::HandleMoveInput(const FVector2D& MoveValue)
{
	if (!Controller)
	{
		return;
	}

	// 컨트롤러 yaw 기준 전방/우측 방향으로 이동한다.
	// UE 표준 컨벤션: MoveValue.Y = 전방(Forward), MoveValue.X = 우측(Right). 축 구성은 IMC에서 맞춘다.
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

	// Jump는 ACharacter 기본 제공.
	Jump();
}

void AFTPlayerCharacter::HandleJumpReleased()
{
	// StopJumping도 ACharacter 기본 제공.
	StopJumping();
}

void AFTPlayerCharacter::HandleSprintPressed()
{
	// 키 상태만 기록한다. 스태미나/크라우치 조건 확인과 실제 속도 적용은 Tick의 UpdateSprintState가 처리.
	bSprintHeld = true;
}

void AFTPlayerCharacter::HandleSprintReleased()
{
	bSprintHeld = false;
}

void AFTPlayerCharacter::HandleCrouchPressed()
{
	// 꾹 누르는 동안 앉는다. CanCrouch() 판정과 캡슐 축소는 ACharacter/CMC가 처리한다.
	Crouch();
}

void AFTPlayerCharacter::HandleCrouchReleased()
{
	// 손을 떼면 일어선다. 머리 위 공간이 없으면 CMC가 공간이 생길 때까지 일어서기를 보류한다.
	UnCrouch();
}

void AFTPlayerCharacter::HandleInteractPressed()
{
	// 실제 트레이스/대상 선정/명령 전송은 상호작용 컴포넌트가 담당한다.
	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AFTPlayerCharacter::HandleInteractReleased()
{
	// 채널형 상호작용 중이면 중단(진행도는 대상에 유지).
	if (InteractionComponent)
	{
		InteractionComponent->StopInteract();
	}
}

void AFTPlayerCharacter::HandleSkillCheckPressed()
{
	// 채널링 중인 대상의 스킬체크 판정으로 전달.
	if (InteractionComponent)
	{
		InteractionComponent->NotifySkillCheckInput();
	}
}

void AFTPlayerCharacter::ApplyMovementSpeed()
{
	// 기본 이동속도는 StatComponent의 MoveSpeed를 권위값으로 쓰고, 스프린트 중이면 배수를 곱한다.
	// 크라우치 상태에서는 CMC가 MaxWalkSpeedCrouched를 쓰므로 이 값은 선 채로 이동할 때 적용된다.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		const float BaseSpeed = StatComponent ? StatComponent->GetMoveSpeed() : 600.0f;
		Movement->MaxWalkSpeed = bIsSprinting ? BaseSpeed * SprintSpeedMultiplier : BaseSpeed;
	}
}

void AFTPlayerCharacter::UpdateSprintState(float DeltaSeconds)
{
	if (!StatComponent)
	{
		return;
	}

	// 탈진 해제: 스태미나가 최대치의 SprintResumeStaminaFraction 이상으로 회복되면 다시 스프린트 가능.
	if (bSprintExhausted && StatComponent->GetStamina() >= StatComponent->GetMaxStamina() * SprintResumeStaminaFraction)
	{
		bSprintExhausted = false;
	}

	// 효과적 스프린트 조건: 키 유지 + 비크라우치 + 비탈진 + 스태미나 잔량.
	bool bSprinting = bSprintHeld && !bIsCrouched && !bSprintExhausted && StatComponent->GetStamina() > 0.0f;

	// 지상에서 실제로 이동 중일 때만 스태미나를 소모하고, 0이 되면 탈진 처리.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const bool bMovingOnGround = Movement && Movement->IsMovingOnGround() && GetVelocity().SizeSquared() > FMath::Square(10.0f);
	if (bSprinting && bMovingOnGround)
	{
		StatComponent->DrainStamina(SprintStaminaCostPerSecond * DeltaSeconds);
		if (StatComponent->GetStamina() <= 0.0f)
		{
			bSprintExhausted = true;
			bSprinting = false;
		}
	}

	// 효과적 스프린트 상태가 바뀌면 속도를 갱신한다.
	if (bSprinting != bIsSprinting)
	{
		bIsSprinting = bSprinting;
		ApplyMovementSpeed();
	}
}

void AFTPlayerCharacter::HandleMoveSpeedChanged(float NewMoveSpeed)
{
	// MoveSpeed 스탯 변경(버프/디버프 등)을 즉시 MaxWalkSpeed에 반영한다.
	ApplyMovementSpeed();
}

void AFTPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// 앉으면 캡슐 중심이 (base 고정 시) HalfHeightAdjust만큼 내려가므로, 카메라를 같은 양만큼 위로 올려 보정한다.
	ApplyCrouchCameraCompensation(+HalfHeightAdjust);
}

void AFTPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	// 일어서면 캡슐 중심이 (base 고정 시) HalfHeightAdjust만큼 올라가므로, 카메라를 같은 양만큼 아래로 내려 보정한다.
	ApplyCrouchCameraCompensation(-HalfHeightAdjust);
}

void AFTPlayerCharacter::ApplyCrouchCameraCompensation(float CameraOffsetDeltaZ)
{
	// 핵심: 엔진은 base를 고정할 때(bCrouchMaintainsBaseLocation=true, 주로 지상)에만 캡슐 중심을 옮긴다.
	// 공중(false)에선 중심이 고정돼 카메라가 실제로 움직이지 않으므로, 보정하면 오히려 시점이 순간이동한다.
	// 캡슐 중심이 실제로 이동한 경우에만 그만큼 반대로 보정하고, 보정량은 Tick에서 매 프레임 0으로 보간한다.
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
	// 기준 상대 위치에 현재 보정량을 더해 카메라 높이를 정한다. 회전은 bUsePawnControlRotation이 담당하므로 위치만 건드린다.
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetRelativeLocation(DefaultCameraRelativeLocation + FVector(0.0f, 0.0f, CrouchCameraOffsetZ));
	}
}

bool AFTPlayerCharacter::TryStartTraversal()
{
	return false;
}
