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

void AFTAICharacterBase::OnDeath()
{
	UE_LOG(LogFTNPC, Log, TEXT("AI character '%s' died."), *GetNameSafe(this));
	if (DeathDespawnDelay <= 0.0f)
	{
		DespawnAfterDeath();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// HP가 0이 된 AI는 잠시 남겨둔 뒤 월드에서 제거한다.
		World->GetTimerManager().SetTimer(
			DeathDespawnTimerHandle,
			this,
			&AFTAICharacterBase::DespawnAfterDeath,
			DeathDespawnDelay,
			false);
	}
	// 이동 정지는 베이스(HandleDeath)가 처리한다. 래그돌/루트 드롭/디스폰 등 AI 전용 후처리는 여기에 추가.
}
