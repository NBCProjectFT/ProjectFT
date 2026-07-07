
#include "FTSecurityCharacter.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

AFTSecurityCharacter::AFTSecurityCharacter()
{
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	CapturePoint = CreateDefaultSubobject<USceneComponent>(TEXT("CapturePoint"));
	CapturePoint->SetupAttachment(GetRootComponent());
}

void AFTSecurityCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultPawnCollisionResponse = GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn);

	// 경비 전용 어빌리티(잡기 등)를 ASC에 부여한다. 싱글이라 권한 검사 없이 바로 부여.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
		{
			if (AbilityClass)
			{
				ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass));
			}
		}
	}
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
