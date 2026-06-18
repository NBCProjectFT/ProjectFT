// Fill out your copyright notice in the Description page of Project Settings.

#include "FTPlayerStatComponent.h"

#include "ProjectFT/Core/FTLogChannels.h"

UFTPlayerStatComponent::UFTPlayerStatComponent()
{
	// 회복(regen)이 필요한 동안에만 틱한다(자체 Tick — 소유 액터 Tick과 독립). 다 차면 스스로 끈다.
	PrimaryComponentTick.bCanEverTick = true;
}

void UFTPlayerStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 디자이너가 설정한 초기값 보정.
	Stats.Health.ClampToMax();
	Stats.Stamina.ClampToMax();
	bIsAlive = !Stats.Health.IsDepleted();

	// 초기 상태 통지(이미 바인딩한 리스너용 — 늦게 바인딩하는 UI는 getter로 초기값을 읽는다).
	OnHealthChanged.Broadcast(Stats.Health.Current, Stats.Health.Max);
	OnStaminaChanged.Broadcast(Stats.Stamina.Current, Stats.Stamina.Max);
	OnMoveSpeedChanged.Broadcast(Stats.MoveSpeed);
	OnDexterityChanged.Broadcast(Stats.Dexterity);
}

void UFTPlayerStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bStillRegenerating = false;

	// 스태미나 회복(마지막 사용 후 지연이 지난 다음부터).
	if (StaminaRegenPerSecond > 0.0f && !Stats.Stamina.IsFull())
	{
		TimeSinceStaminaUse += DeltaTime;
		if (TimeSinceStaminaUse >= StaminaRegenDelay)
		{
			Stats.Stamina.Current = FMath::Min(Stats.Stamina.Max, Stats.Stamina.Current + StaminaRegenPerSecond * DeltaTime);
			OnStaminaChanged.Broadcast(Stats.Stamina.Current, Stats.Stamina.Max);
		}
		bStillRegenerating = true; // 회복 중이거나 지연 대기 중이면 계속 틱한다.
	}

	// 체력 회복(기본 0이라 보통 비활성).
	if (HealthRegenPerSecond > 0.0f && bIsAlive && !Stats.Health.IsFull())
	{
		Stats.Health.Current = FMath::Min(Stats.Health.Max, Stats.Health.Current + HealthRegenPerSecond * DeltaTime);
		OnHealthChanged.Broadcast(Stats.Health.Current, Stats.Health.Max);
		bStillRegenerating = true;
	}

	// 더 회복할 게 없으면 틱을 끈다(소모 시 다시 켜진다).
	if (!bStillRegenerating)
	{
		SetComponentTickEnabled(false);
	}
}

void UFTPlayerStatComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || !bIsAlive || Stats.Health.IsFull())
	{
		return;
	}

	Stats.Health.Current = FMath::Min(Stats.Health.Max, Stats.Health.Current + Amount);
	OnHealthChanged.Broadcast(Stats.Health.Current, Stats.Health.Max);
}

void UFTPlayerStatComponent::ApplyDamage(float Amount)
{
	if (Amount <= 0.0f || !bIsAlive)
	{
		return;
	}

	Stats.Health.Current = FMath::Max(0.0f, Stats.Health.Current - Amount);
	OnHealthChanged.Broadcast(Stats.Health.Current, Stats.Health.Max);

	if (Stats.Health.IsDepleted())
	{
		bIsAlive = false;
		UE_LOG(LogFTPlayer, Log, TEXT("'%s' died (health depleted)."), *GetNameSafe(GetOwner()));
		// 사망 후처리(레벨 전환 등)는 직접 하지 않고 리스너(캐릭터/GameFlow)가 OnDied로 받아 처리한다.
		OnDied.Broadcast();
	}
	else if (HealthRegenPerSecond > 0.0f)
	{
		SetComponentTickEnabled(true);
	}
}

void UFTPlayerStatComponent::SetMaxHealth(float NewMax, bool bRefill)
{
	Stats.Health.Max = FMath::Max(0.0f, NewMax);
	if (bRefill)
	{
		Stats.Health.Current = Stats.Health.Max;
	}
	Stats.Health.ClampToMax();
	OnHealthChanged.Broadcast(Stats.Health.Current, Stats.Health.Max);
}

bool UFTPlayerStatComponent::TryConsumeStamina(float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}
	if (Stats.Stamina.Current < Amount)
	{
		return false;
	}

	Stats.Stamina.Current -= Amount;
	TimeSinceStaminaUse = 0.0f;
	OnStaminaChanged.Broadcast(Stats.Stamina.Current, Stats.Stamina.Max);
	SetComponentTickEnabled(true);
	return true;
}

void UFTPlayerStatComponent::DrainStamina(float Amount)
{
	if (Amount <= 0.0f || Stats.Stamina.IsDepleted())
	{
		return;
	}

	Stats.Stamina.Current = FMath::Max(0.0f, Stats.Stamina.Current - Amount);
	TimeSinceStaminaUse = 0.0f;
	OnStaminaChanged.Broadcast(Stats.Stamina.Current, Stats.Stamina.Max);
	SetComponentTickEnabled(true);
}

void UFTPlayerStatComponent::RestoreStamina(float Amount)
{
	if (Amount <= 0.0f || Stats.Stamina.IsFull())
	{
		return;
	}

	Stats.Stamina.Current = FMath::Min(Stats.Stamina.Max, Stats.Stamina.Current + Amount);
	OnStaminaChanged.Broadcast(Stats.Stamina.Current, Stats.Stamina.Max);
}

void UFTPlayerStatComponent::SetMaxStamina(float NewMax, bool bRefill)
{
	Stats.Stamina.Max = FMath::Max(0.0f, NewMax);
	if (bRefill)
	{
		Stats.Stamina.Current = Stats.Stamina.Max;
	}
	Stats.Stamina.ClampToMax();
	OnStaminaChanged.Broadcast(Stats.Stamina.Current, Stats.Stamina.Max);
}

void UFTPlayerStatComponent::SetMoveSpeed(float NewMoveSpeed)
{
	NewMoveSpeed = FMath::Max(0.0f, NewMoveSpeed);
	if (FMath::IsNearlyEqual(Stats.MoveSpeed, NewMoveSpeed))
	{
		return;
	}

	Stats.MoveSpeed = NewMoveSpeed;
	OnMoveSpeedChanged.Broadcast(Stats.MoveSpeed);
}

void UFTPlayerStatComponent::SetDexterity(float NewDexterity)
{
	NewDexterity = FMath::Max(0.0f, NewDexterity);
	if (FMath::IsNearlyEqual(Stats.Dexterity, NewDexterity))
	{
		return;
	}

	Stats.Dexterity = NewDexterity;
	OnDexterityChanged.Broadcast(Stats.Dexterity);
}
