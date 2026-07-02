#include "FTAICharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/Core/FTLogChannels.h"

AFTAICharacterBase::AFTAICharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFTAICharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AttributeSet)
	{
		// 에디터에서 설정한 초기 스탯을 AttributeSet에 적용한다.
		AttributeSet->SetMaxHealth(InitialHealth);
		AttributeSet->SetHealth(InitialHealth);
		AttributeSet->SetMoveSpeed(InitialMoveSpeed);
		
		// 변경된 이동속도를 CharacterMovementComponent에 반영한다.
		ApplyMovementSpeed();
	}
}

void AFTAICharacterBase::SetMoveSpeed(float NewSpeed)
{
	if (AttributeSet)
	{
		// 이동 속성 값을 갱신한 뒤 실제 이동 컴포넌트에도 적용한다.
		AttributeSet->SetMoveSpeed(NewSpeed);
		ApplyMovementSpeed();
	}
}

void AFTAICharacterBase::HandleDeath()
{
	UE_LOG(LogFTNPC, Log, TEXT("AI character '%s' died."), *GetNameSafe(this));

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// 사망한 AI가 더 이상 이동하지 않도록 비활성화한다.
		Movement->DisableMovement();
	}
	
	Super::HandleDeath();
}
