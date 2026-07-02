#include "FTSecurityChaseGaugeComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"

UFTSecurityChaseGaugeComponent::UFTSecurityChaseGaugeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFTSecurityChaseGaugeComponent::StartListening()
{
	Super::StartListening();

	// 보안요원 호출, 타겟 감지, 타겟 놓침 메시지를 수신해 추격 게이지 상태를 갱신한다.
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityCalled,
		this,
		&ThisClass::OnSecurityCalled
	));
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityTargetSeen,
		this,
		&ThisClass::OnSecurityTargetSeen
	));
	AddListenerHandle(MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityTargetLost,
		this,
		&ThisClass::OnSecurityTargetLost
	));
}

void UFTSecurityChaseGaugeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bChaseActive)
	{
		return;
	}

	// 한 명이라도 대상을 보고 있으면 추격 게이지를 최대 상태로 유지한다.
	RemoveInvalidSecurityActors();
	if (!SecuritiesSeeingTarget.IsEmpty())
	{
		return;
	}

	SetChaseGauge(CurrentChaseGauge - ChaseGaugeDecayPerSecond * DeltaTime);
}

void UFTSecurityChaseGaugeComponent::OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	// 새 신고는 기존 감소 상태와 관계없이 추격을 다시 최대치로 활성화한다.
	TargetActor = Payload.TargetActor;
	LastKnownLocation = Payload.ReportLocation;
	bChaseActive = true;
	SetChaseGauge(MaxChaseGauge);
}

void UFTSecurityChaseGaugeComponent::OnSecurityTargetSeen(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	if (!Payload.SecurityActor)
	{
		return;
	}

	SecuritiesSeeingTarget.Add(TWeakObjectPtr<AActor>(Payload.SecurityActor.Get()));
	TargetActor = Payload.TargetActor;
	LastKnownLocation = Payload.LastKnownLocation;
	bChaseActive = true;

	// 플레이어를 다시 목격하면 감소 중이던 게이지를 즉시 최대치로 복원한다.
	if (!FMath::IsNearlyEqual(CurrentChaseGauge, MaxChaseGauge))
	{
		SetChaseGauge(MaxChaseGauge);
		return;
	}

	BroadcastGaugeChanged();
}

void UFTSecurityChaseGaugeComponent::OnSecurityTargetLost(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	// 여러 보안요원 중 메시지를 보낸 보안요원만 감지 목록에서 제거한다.
	if (Payload.SecurityActor)
	{
		SecuritiesSeeingTarget.Remove(TWeakObjectPtr<AActor>(Payload.SecurityActor.Get()));
	}

	if (!Payload.LastKnownLocation.IsNearlyZero())
	{
		LastKnownLocation = Payload.LastKnownLocation;
	}

	if (bChaseActive)
	{
		BroadcastGaugeChanged();
	}
}

void UFTSecurityChaseGaugeComponent::SetChaseGauge(float NewChaseGauge)
{
	// 모든 게이지 변경은 이 함수를 거쳐 범위 제한과 메시지 발행 순서를 통일한다.
	const float ClampedChaseGauge = FMath::Clamp(NewChaseGauge, 0.0f, MaxChaseGauge);
	if (FMath::IsNearlyEqual(CurrentChaseGauge, ClampedChaseGauge))
	{
		return;
	}

	CurrentChaseGauge = ClampedChaseGauge;
	BroadcastGaugeChanged();

	if (CurrentChaseGauge <= 0.0f && bChaseActive)
	{
		// 활성 추격이 0에 도달한 경우에만 종료 메시지를 한 번 발행한다.
		bChaseActive = false;
		SecuritiesSeeingTarget.Reset();
		BroadcastChaseEnded();
	}
}

void UFTSecurityChaseGaugeComponent::BroadcastGaugeChanged()
{
	// UI와 보안요원이 동일한 추격 상태를 읽을 수 있도록 현재 스냅샷을 전달한다.
	FFTSecurityChaseGaugePayloadStruct Payload;
	Payload.TargetActor = TargetActor;
	Payload.LastKnownLocation = LastKnownLocation;
	Payload.ChaseGauge = CurrentChaseGauge;
	Payload.ChaseGaugeRatio = GetChaseGaugeRatio();
	Payload.bHasSeenTarget = !SecuritiesSeeingTarget.IsEmpty();

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityChaseGaugeChanged, Payload);
}

void UFTSecurityChaseGaugeComponent::BroadcastChaseEnded()
{
	FFTSecurityChaseGaugePayloadStruct Payload;
	Payload.TargetActor = TargetActor;
	Payload.LastKnownLocation = LastKnownLocation;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityChaseEnded, Payload);
}

void UFTSecurityChaseGaugeComponent::RemoveInvalidSecurityActors()
{
	// 파괴된 보안요원이 게이지 감소를 계속 막지 않도록 약한 참조를 정리한다.
	for (auto SecurityIterator = SecuritiesSeeingTarget.CreateIterator(); SecurityIterator; ++SecurityIterator)
	{
		if (!SecurityIterator->IsValid())
		{
			SecurityIterator.RemoveCurrent();
		}
	}
}

void UFTSecurityChaseGaugeComponent::ResetChaseGauge()
{
	const bool bWasChaseActive = bChaseActive;

	// 수신자가 완전히 초기화된 상태를 받도록 메시지 발행 전에 모든 값을 정리한다.
	CurrentChaseGauge = 0.0f;
	bChaseActive = false;
	SecuritiesSeeingTarget.Reset();
	TargetActor = nullptr;
	LastKnownLocation = FVector::ZeroVector;

	BroadcastGaugeChanged();

	if (bWasChaseActive)
	{
		BroadcastChaseEnded();
	}
}

float UFTSecurityChaseGaugeComponent::GetChaseGauge() const
{
	return CurrentChaseGauge;
}

float UFTSecurityChaseGaugeComponent::GetChaseGaugeRatio() const
{
	return MaxChaseGauge > 0.0f ? CurrentChaseGauge / MaxChaseGauge : 0.0f;
}

bool UFTSecurityChaseGaugeComponent::IsChaseActive() const
{
	return bChaseActive;
}

bool UFTSecurityChaseGaugeComponent::IsAnySecuritySeeingTarget() const
{
	return !SecuritiesSeeingTarget.IsEmpty();
}
