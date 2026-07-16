#include "FTCrosshairComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectFT/Data/FTHitScanDataAsset.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTHUDViewModel.h"

UFTCrosshairComponent::UFTCrosshairComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UFTCrosshairComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyDefaultCrosshair();
	UpdateCrosshair(0.0f);
}

void UFTCrosshairComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateCrosshair(DeltaTime);
}

void UFTCrosshairComponent::SetActiveItemData(const UFTItemDataAsset* ItemData)
{
	ActiveHitScanData = Cast<UFTHitScanDataAsset>(ItemData);

	if (!ActiveHitScanData)
	{
		ApplyDefaultCrosshair();
		return;
	}

	CrosshairState = ActiveHitScanData->CrosshairData;
	CrosshairState.bVisible = true;
	if (CrosshairState.SpreadMax <= 0.0f)
	{
		CrosshairState.SpreadMax = DefaultSpreadMax;
	}

	UpdateCrosshair(0.0f);
}

void UFTCrosshairComponent::ClearCrosshair()
{
	ActiveHitScanData = nullptr;
	ApplyDefaultCrosshair();
}

void UFTCrosshairComponent::ApplyDefaultCrosshair()
{
	ResetCrosshairFactors();

	if (bShowDefaultCrosshair)
	{
		CrosshairState = DefaultCrosshairData;
		CrosshairState.bVisible = true;
		if (CrosshairState.SpreadMax <= 0.0f)
		{
			CrosshairState.SpreadMax = DefaultSpreadMax;
		}
	}
	else
	{
		CrosshairState = FTCrosshairStateStruct();
	}

	PushCrosshairStateToHUD();
}

void UFTCrosshairComponent::ResetCrosshairFactors()
{
	CrosshairVelocityFactor = 0.0f;
	CrosshairInAirFactor = 0.0f;
	CrosshairCrouchFactor = 0.0f;
	CrosshairAimFactor = 0.0f;
	CrosshairShootingFactor = 0.0f;
}

void UFTCrosshairComponent::PushCrosshairStateToHUD() const
{
	if (UFTHUDViewModel* HUDViewModel = ResolveHUDViewModel())
	{
		HUDViewModel->SetCrosshairState(CrosshairState);
	}
}

void UFTCrosshairComponent::NotifyFired()
{
	if (!CrosshairState.bVisible)
	{
		return;
	}

	CrosshairShootingFactor = FMath::Max(CrosshairShootingFactor, ShootingImpulse);
	UpdateCrosshair(0.0f);
}

void UFTCrosshairComponent::UpdateCrosshair(float DeltaTime)
{
	if (!CrosshairState.bVisible)
	{
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !Movement)
	{
		return;
	}

	const FVector HorizontalVelocity(Character->GetVelocity().X, Character->GetVelocity().Y, 0.0f);
	const float MaxWalkSpeed = FMath::Max(Movement->MaxWalkSpeed, 1.0f);
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		FVector2D(0.0f, MaxWalkSpeed),
		FVector2D(0.0f, 1.0f),
		HorizontalVelocity.Size());

	const float InAirTarget = Movement->IsFalling() ? InAirTargetFactor : 0.0f;
	const float InAirInterpSpeed = Movement->IsFalling() ? 2.25f : 30.0f;
	CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, InAirTarget, DeltaTime, InAirInterpSpeed);

	const float CrouchTarget = Movement->IsCrouching() ? CrouchTargetFactor : 0.0f;
	CrosshairCrouchFactor = FMath::FInterpTo(CrosshairCrouchFactor, CrouchTarget, DeltaTime, 30.0f);

	CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.0f, DeltaTime, 30.0f);
	CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0f, DeltaTime, 20.0f);

	CrosshairState.Spread = BaseSpread
		+ CrosshairVelocityFactor
		+ CrosshairInAirFactor
		- CrosshairCrouchFactor
		- CrosshairAimFactor
		+ CrosshairShootingFactor;
	CrosshairState.Spread = FMath::Max(0.0f, CrosshairState.Spread);

	PushCrosshairStateToHUD();
}

UFTHUDViewModel* UFTCrosshairComponent::ResolveHUDViewModel() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UFTUIManagerSubsystem* UIManagerSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTUIManagerSubsystem>() : nullptr;
	return UIManagerSubsystem ? UIManagerSubsystem->HUDViewModel : nullptr;
}
