#include "FTAICharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/Core/FTLogChannels.h"

AFTAICharacterBase::AFTAICharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFTAICharacterBase::DespawnAfterDeath()
{
	Destroy();
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
		
		// 변경된 이동 속도를 CharacterMovementComponent에 반영한다.
		ApplyMovementSpeed();
	}
}

void AFTAICharacterBase::SetMoveSpeed(float NewSpeed)
{
	if (AttributeSet)
	{
		// 이동 속성 값을 갱신하고 실제 이동 컴포넌트에도 적용한다.
		AttributeSet->SetMoveSpeed(NewSpeed);
		ApplyMovementSpeed();
	}
}

void AFTAICharacterBase::OnDeath()
{
	UE_LOG(LogFTNPC, Log, TEXT("AI character '%s' died."), *GetNameSafe(this));
	// 이동 정지는 공통 HandleDeath에서 처리한다.
	// 쓰러짐 연출과 제거 타이밍은 StateTree의 KnockedOut 상태가 담당한다.
}
