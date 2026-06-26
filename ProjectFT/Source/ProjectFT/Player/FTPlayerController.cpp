// Fill out your copyright notice in the Description page of Project Settings.

#include "FTPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Interface/FTInputInterface.h"

void AFTPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem || !DefaultMappingContext)
	{
		return;
	}

	if (bClearMappingsBeforeAdd)
	{
		Subsystem->ClearAllMappings();
	}

	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

void AFTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFTPlayerController::OnMoveTriggered);
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFTPlayerController::OnLookTriggered);
	}

	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AFTPlayerController::OnJumpStarted);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFTPlayerController::OnJumpCompleted);
	}

	if (SprintAction)
	{
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AFTPlayerController::OnSprintStarted);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFTPlayerController::OnSprintCompleted);
	}
	
	if (CrouchAction)
	{
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &AFTPlayerController::OnCrouchStarted);
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AFTPlayerController::OnCrouchCompleted);
	}

	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AFTPlayerController::OnInteractStarted);
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Completed, this, &AFTPlayerController::OnInteractCompleted);
	}

	if (SkillCheckAction)
	{
		EnhancedInput->BindAction(SkillCheckAction, ETriggerEvent::Started, this, &AFTPlayerController::OnSkillCheckStarted);
	}

	if (UseItemAction)
	{
		EnhancedInput->BindAction(UseItemAction, ETriggerEvent::Started, this, &AFTPlayerController::OnUseItemStarted);
	}

	if (QuickSlot1Action)
	{
		EnhancedInput->BindAction(QuickSlot1Action, ETriggerEvent::Started, this, &AFTPlayerController::OnQuickSlot1Started);
	}

	if (QuickSlot2Action)
	{
		EnhancedInput->BindAction(QuickSlot2Action, ETriggerEvent::Started, this, &AFTPlayerController::OnQuickSlot2Started);
	}
	
	if (QuickSlot3Action)
	{
		EnhancedInput->BindAction(QuickSlot3Action, ETriggerEvent::Started, this, &AFTPlayerController::OnQuickSlot3Started);
	}

#if !UE_BUILD_SHIPPING
	// [Temp/Debug] IA 에셋/IMC 매핑이 아직 없어도 테이저를 바로 쏴보기 위한 하드코딩 키. T = 퀵슬롯0 선택 후 사용.
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AFTPlayerController::OnDebugUseQuickSlot0);
	}
#endif
}

void AFTPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CachedInputPawn = InPawn;
	CachedLocomotionInput = Cast<IFTInputInterface>(InPawn);

	if (!CachedLocomotionInput)
	{
		UE_LOG(LogFTPlayer, Warning,
			TEXT("Possessed pawn '%s' does not implement IFTInputInterface; locomotion input will be ignored."),
			*GetNameSafe(InPawn));
	}
}

void AFTPlayerController::OnUnPossess()
{
	CachedInputPawn = nullptr;
	CachedLocomotionInput = nullptr;

	Super::OnUnPossess();
}

void AFTPlayerController::OnMoveTriggered(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleMoveInput(Value.Get<FVector2D>());
	}
}

void AFTPlayerController::OnLookTriggered(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleLookInput(Value.Get<FVector2D>());
	}
}

void AFTPlayerController::OnJumpStarted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleJumpPressed();
	}
}

void AFTPlayerController::OnJumpCompleted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleJumpReleased();
	}
}

void AFTPlayerController::OnSprintStarted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleSprintPressed();
	}
}

void AFTPlayerController::OnSprintCompleted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleSprintReleased();
	}
}

void AFTPlayerController::OnCrouchStarted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleCrouchPressed();
	}
}

void AFTPlayerController::OnCrouchCompleted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleCrouchReleased();
	}
}

void AFTPlayerController::OnInteractStarted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleInteractPressed();
	}
}

void AFTPlayerController::OnInteractCompleted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleInteractReleased();
	}
}

void AFTPlayerController::OnSkillCheckStarted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleSkillCheckPressed();
	}
}

void AFTPlayerController::OnUseItemStarted(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleUseItemPressed();
	}
}

void AFTPlayerController::OnQuickSlot1Started(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleSelectQuickSlot(0);
	}
}

void AFTPlayerController::OnQuickSlot2Started(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleSelectQuickSlot(1);
	}
}

void AFTPlayerController::OnQuickSlot3Started(const FInputActionValue& Value)
{
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleSelectQuickSlot(2);
	}
}

void AFTPlayerController::OnDebugUseQuickSlot0()
{
	// [Temp/Debug] 퀵슬롯0을 선택한 뒤 사용 — 테이저 DA를 MockQuickSlots[0]에 넣고 T를 누르면 발사된다.
	if (CachedLocomotionInput)
	{
		CachedLocomotionInput->HandleSelectQuickSlot(0);
		CachedLocomotionInput->HandleUseItemPressed();
	}
}