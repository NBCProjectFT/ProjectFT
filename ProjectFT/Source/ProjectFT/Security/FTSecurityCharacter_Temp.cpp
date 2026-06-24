#include "FTSecurityCharacter_Temp.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/Core/FTLogChannels.h"

AFTSecurityCharacter_Temp::AFTSecurityCharacter_Temp()
{
	PrimaryActorTick.bCanEverTick = false;

	// MaxWalkSpeed 초기값(프리뷰)·ASC·공용 AttributeSet은 베이스(AFTCharacterBase)가 처리한다.
	// 경비 고유 속도(300)는 InitialMoveSpeed 한 곳에만 두고 BeginPlay에서 속성으로 주입한다.
}

void AFTSecurityCharacter_Temp::BeginPlay()
{
	// 베이스: InitAbilityActorInfo + MoveSpeed→MaxWalkSpeed 파생 + 스턴 이동정지 + OnOutOfHealth→HandleDeath.
	Super::BeginPlay();

	if (AttributeSet)
	{
		// 초기 스탯 주입. MoveSpeed를 바꾸면 베이스가 바인딩한 델리게이트가 MaxWalkSpeed에 반영한다(별도 코드 불필요).
		AttributeSet->SetMaxHealth(InitialHealth);
		AttributeSet->SetHealth(InitialHealth);
		AttributeSet->SetMoveSpeed(InitialMoveSpeed);
	}
}

void AFTSecurityCharacter_Temp::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AFTSecurityCharacter_Temp::SetMoveSpeed(float NewSpeed)
{
	// CMC 직접 X → MoveSpeed 속성을 바꾼다. 베이스의 변경 델리게이트가 MaxWalkSpeed에 반영하고 버프와도 합성된다.
	if (AttributeSet)
	{
		AttributeSet->SetMoveSpeed(NewSpeed);
	}
}

void AFTSecurityCharacter_Temp::HandleDeath()
{
	UE_LOG(LogFTNPC, Log, TEXT("[Temp] Security '%s' died (health depleted)."), *GetNameSafe(this));

	// (예시) 사망 시 이동 정지. 실제 사망 연출/제거/GameFlow 통지는 담당자 몫.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->DisableMovement();
	}
}
