// Fill out your copyright notice in the Description page of Project Settings.

#include "FTCaptureEscapeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTCaptureEscapeComponent::UFTCaptureEscapeComponent()
{
	// 붙잡힌 동안에만 틱한다(게이지 자연 감소용).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFTCaptureEscapeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 자연 감소: 연타를 멈추면 게이지가 줄어든다(탈출하려면 계속 연타하도록).
	if (bCaptured && !bEscaped && StruggleDecayPerSecond > 0.0f && EscapeProgress > 0.0f)
	{
		SetProgress(EscapeProgress - StruggleDecayPerSecond * DeltaTime);
	}
}

void UFTCaptureEscapeComponent::BeginCapture(AActor* InCaptor, USceneComponent* InAttachPoint)
{
	if (bCaptured)
	{
		return;
	}

	bCaptured = true;
	bEscaped = false;
	EscapeProgress = 0.0f;
	LastStruggleSign = 0.0f;
	Captor = InCaptor;

	// 붙잡힘 상태 태그(이동/시점/아이템 차단 판정의 단일 소스).
	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystem())
	{
		ASC->AddLooseGameplayTag(TAG_FT_State_Captured);
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		// 스스로 이동 못 하게 CMC 정지.
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}

		// 캡슐이 경비/월드와 부딪혀 이송을 방해하지 않도록 콜리전을 잠시 끈다(원래 설정은 저장 후 EndCapture에서 복구).
		if (UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
		{
			SavedCollisionEnabled = Capsule->GetCollisionEnabled();
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// 붙잡은 지점(경비 CapturePoint)에 부착해 경비 이동을 따라가게 한다.
		if (InAttachPoint)
		{
			OwnerCharacter->AttachToComponent(InAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}

	SetComponentTickEnabled(true);
}

void UFTCaptureEscapeComponent::EndCapture()
{
	if (!bCaptured)
	{
		return;
	}

	bCaptured = false;
	SetComponentTickEnabled(false);

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		OwnerCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		if (UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(SavedCollisionEnabled);
		}

		// 이동 복구(다시 걷기 — 공중이면 낙하 후 착지).
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}

	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystem())
	{
		ASC->RemoveLooseGameplayTag(TAG_FT_State_Captured);
	}

	EscapeProgress = 0.0f;
	LastStruggleSign = 0.0f;
	Captor = nullptr;
}

void UFTCaptureEscapeComponent::AddStruggleInput(float MoveAxisX)
{
	if (!bCaptured || bEscaped)
	{
		return;
	}

	// 데드존 밖일 때만 방향으로 인정. 직전 인정 방향과 반대가 되면 "좌우 전환(flip)"으로 게이지 상승.
	float Sign = 0.0f;
	if (MoveAxisX > StruggleInputDeadzone)
	{
		Sign = 1.0f;
	}
	else if (MoveAxisX < -StruggleInputDeadzone)
	{
		Sign = -1.0f;
	}

	if (Sign != 0.0f)
	{
		if (LastStruggleSign != 0.0f && Sign != LastStruggleSign)
		{
			SetProgress(EscapeProgress + StruggleGainPerFlip);
		}
		LastStruggleSign = Sign;
	}
}

void UFTCaptureEscapeComponent::SetProgress(float NewProgress)
{
	EscapeProgress = FMath::Clamp(NewProgress, 0.0f, 1.0f);

	if (!bEscaped && EscapeProgress >= 1.0f)
	{
		bEscaped = true;
		// 어빌리티가 이걸 받아 성공 처리(해방 + 경비 스턴)한다. EndCapture는 어빌리티가 호출한다.
		OnEscaped.Broadcast();
	}
}

UAbilitySystemComponent* UFTCaptureEscapeComponent::GetOwnerAbilitySystem() const
{
	if (const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return AbilityOwner->GetAbilitySystemComponent();
	}
	return nullptr;
}
