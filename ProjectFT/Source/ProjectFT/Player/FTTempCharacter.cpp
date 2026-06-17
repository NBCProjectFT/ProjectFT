// Fill out your copyright notice in the Description page of Project Settings.

#include "FTTempCharacter.h"
#include "EnhancedInputComponent.h"
#include "FTTempController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"

// Sets default values
AFTTempCharacter::AFTTempCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// create the camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = 100.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;

	// create the orbiting camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	NormalSpeed = 600.0f;
	SprintSpeedMultiplier = 1.5f;
	SprintSpeed = NormalSpeed * SprintSpeedMultiplier;

	GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
}

// Called when the game starts or when spawned
void AFTTempCharacter::BeginPlay() 
{ 
	Super::BeginPlay(); 
}

// Called every frame
void AFTTempCharacter::Tick(float DeltaTime) 
{ 
	Super::Tick(DeltaTime); 
}

// Called to bind functionality to input
void AFTTempCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (AFTTempController* PlayerController = Cast<AFTTempController>(GetController()))
		{
			if (PlayerController->InteractAction)
			{
				EnhancedInput->BindAction(
					PlayerController->InteractAction,
					ETriggerEvent::Triggered,
					this,
					&AFTTempCharacter::Interaction
				);
			}
			
			if (PlayerController->MoveAction)
			{
				// IA_Move 액션 키를 "키를 누르고 있는 동안" Move() 호출
				EnhancedInput->BindAction(
					PlayerController->MoveAction,
					ETriggerEvent::Triggered,
					this,
					&AFTTempCharacter::Move
				);
			}

			if (PlayerController->JumpAction)
			{
				// IA_Jump 액션 키를 "키를 누르고 있는 동안" StartJump() 호출
				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Triggered,
					this,
					&AFTTempCharacter::StartJump
				);

				// IA_Jump 액션 키에서 "손을 뗀 순간" StopJump() 호출
				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Completed,
					this,
					&AFTTempCharacter::StopJump
				);
			}

			if (PlayerController->LookAction)
			{
				// IA_Look 액션 마우스가 "움직일 때" Look() 호출
				EnhancedInput->BindAction(
					PlayerController->LookAction,
					ETriggerEvent::Triggered,
					this,
					&AFTTempCharacter::Look
				);
			}

			if (PlayerController->SprintAction)
			{
				// IA_Sprint 액션 키를 "누르고 있는 동안" StartSprint() 호출
				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Triggered,
					this,
					&AFTTempCharacter::StartSprint
				);
				// IA_Sprint 액션 키에서 "손을 뗀 순간" StopSprint() 호출
				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Completed,
					this,
					&AFTTempCharacter::StopSprint
				);
			}
		}
	}
}

void AFTTempCharacter::Move(const FInputActionValue& value)
{
	if (!Controller) return;

	const FVector2D MoveInput = value.Get<FVector2D>();

	if (!FMath::IsNearlyZero(MoveInput.X)) { AddMovementInput(GetActorForwardVector(), MoveInput.X); }
	if (!FMath::IsNearlyZero(MoveInput.Y)) { AddMovementInput(GetActorRightVector(), MoveInput.Y); }
}

void AFTTempCharacter::StartJump(const FInputActionValue& value)
{
	// Jump 함수는 Character가 기본 제공
	if (value.Get<bool>()) { Jump(); }
}

void AFTTempCharacter::StopJump(const FInputActionValue& value)
{
	// StopJumping 함수도 Character가 기본 제공
	if (!value.Get<bool>()) { StopJumping(); }
}

void AFTTempCharacter::Look(const FInputActionValue& value)
{
	FVector2D LookInput = value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AFTTempCharacter::StopSprint(const FInputActionValue& value)
{
	if (GetCharacterMovement()) { GetCharacterMovement()->MaxWalkSpeed = NormalSpeed; }
}

void AFTTempCharacter::StartSprint(const FInputActionValue& value)
{
	if (GetCharacterMovement()) { GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
}

void AFTTempCharacter::Interaction(const FInputActionValue& value)
{
	UE_LOG(LogFTCore, Warning, TEXT("Interaction"));
}
