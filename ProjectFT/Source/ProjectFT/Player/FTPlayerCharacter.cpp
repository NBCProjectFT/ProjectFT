// Fill out your copyright notice in the Description page of Project Settings.


#include "../Player/FTPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	template <typename ModifierType>
	ModifierType* AddInputModifier(UInputMappingContext* MappingContext, FEnhancedActionKeyMapping& Mapping)
	{
		ModifierType* Modifier = NewObject<ModifierType>(MappingContext);
		Mapping.Modifiers.Add(Modifier);
		return Modifier;
	}
}

// Sets default values
AFTPlayerCharacter::AFTPlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	Movement->JumpZVelocity = 600.0f;
	Movement->AirControl = 0.25f;
	Movement->MaxWalkSpeed = 500.0f;
	Movement->BrakingDecelerationWalking = 2048.0f;

	CharacterBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CharacterBody"));
	CharacterBody->SetupAttachment(RootComponent);
	CharacterBody->SetRelativeLocation(FVector(0.0f, 0.0f, -45.0f));
	CharacterBody->SetRelativeScale3D(FVector(0.75f, 0.75f, 1.8f));
	CharacterBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMeshRef(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (BodyMeshRef.Succeeded())
	{
		CharacterBody->SetStaticMesh(BodyMeshRef.Object);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 360.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 60.0f, 70.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	LookAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Look"));
	LookAction->ValueType = EInputActionValueType::Axis2D;

	JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Jump"));
	JumpAction->ValueType = EInputActionValueType::Boolean;

	DefaultMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("IMC_Default"));

	FEnhancedActionKeyMapping& MoveForwardMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::W);
	AddInputModifier<UInputModifierSwizzleAxis>(DefaultMappingContext, MoveForwardMapping)->Order = EInputAxisSwizzle::YXZ;

	FEnhancedActionKeyMapping& MoveBackwardMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
	AddInputModifier<UInputModifierNegate>(DefaultMappingContext, MoveBackwardMapping);
	AddInputModifier<UInputModifierSwizzleAxis>(DefaultMappingContext, MoveBackwardMapping)->Order = EInputAxisSwizzle::YXZ;

	DefaultMappingContext->MapKey(MoveAction, EKeys::D);

	FEnhancedActionKeyMapping& MoveLeftMapping = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
	AddInputModifier<UInputModifierNegate>(DefaultMappingContext, MoveLeftMapping);

	DefaultMappingContext->MapKey(LookAction, EKeys::MouseX);

	FEnhancedActionKeyMapping& MouseYMapping = DefaultMappingContext->MapKey(LookAction, EKeys::MouseY);
	AddInputModifier<UInputModifierNegate>(DefaultMappingContext, MouseYMapping);
	AddInputModifier<UInputModifierSwizzleAxis>(DefaultMappingContext, MouseYMapping)->Order = EInputAxisSwizzle::YXZ;

	DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
}

// Called when the game starts or when spawned
void AFTPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AFTPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AFTPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFTPlayerCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFTPlayerCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AFTPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (Controller == nullptr)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsNearlyZero())
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FRotationMatrix YawMatrix(YawRotation);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), MovementVector.X);
}

void AFTPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (LookAxisVector.IsNearlyZero())
	{
		return;
	}

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

