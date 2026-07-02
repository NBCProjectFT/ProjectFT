
#include "FTSecurityCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

AFTSecurityCharacter::AFTSecurityCharacter()
{
	CapturePoint = CreateDefaultSubobject<USceneComponent>(TEXT("CapturePoint"));
	CapturePoint->SetupAttachment(GetRootComponent());
}

void AFTSecurityCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultPawnCollisionResponse = GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn);
}

void AFTSecurityCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AFTSecurityCharacter::SetMoveSpeed(float NewSpeed)
{
	Super::SetMoveSpeed(NewSpeed);
}

void AFTSecurityCharacter::IgnorePawnCollisionForDuration(float Duration)
{
	SetPawnCollisionIgnored(true);
	GetWorldTimerManager().ClearTimer(PawnCollisionRestoreTimerHandle);

	if (Duration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			PawnCollisionRestoreTimerHandle,
			this,
			&ThisClass::RestorePawnCollision,
			Duration,
			false
		);
	}
}

void AFTSecurityCharacter::SetPawnCollisionIgnored(bool bIgnored)
{
	GetCapsuleComponent()->SetCollisionResponseToChannel(
		ECC_Pawn,
		bIgnored ? ECR_Ignore : DefaultPawnCollisionResponse
	);
}

void AFTSecurityCharacter::RestorePawnCollision()
{
	GetWorldTimerManager().ClearTimer(PawnCollisionRestoreTimerHandle);
	SetPawnCollisionIgnored(false);
}

USceneComponent* AFTSecurityCharacter::GetCapturePointComponent() const
{
	return CapturePoint;
}

FVector AFTSecurityCharacter::GetCapturePointLocation() const
{
	return CapturePoint ? CapturePoint->GetComponentLocation() : GetActorLocation();
}
