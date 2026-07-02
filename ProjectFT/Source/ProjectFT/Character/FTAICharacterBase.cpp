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
		AttributeSet->SetMaxHealth(InitialHealth);
		AttributeSet->SetHealth(InitialHealth);
		AttributeSet->SetMoveSpeed(InitialMoveSpeed);
		ApplyMovementSpeed();
	}
}

void AFTAICharacterBase::SetMoveSpeed(float NewSpeed)
{
	if (AttributeSet)
	{
		AttributeSet->SetMoveSpeed(NewSpeed);
		ApplyMovementSpeed();
	}
}

void AFTAICharacterBase::OnDeath()
{
	UE_LOG(LogFTNPC, Log, TEXT("AI character '%s' died."), *GetNameSafe(this));
	// 이동 정지는 베이스(HandleDeath)가 처리한다. 래그돌/루트 드롭/디스폰 등 AI 전용 후처리는 여기에 추가.
}
