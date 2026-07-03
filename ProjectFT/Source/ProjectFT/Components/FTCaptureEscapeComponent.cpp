// Fill out your copyright notice in the Description page of Project Settings.

#include "FTCaptureEscapeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

	// 힘싸움: 매 틱 플레이어 자연증가(StrugglePassiveGainPerSecond)와 AI 자연감소(EscapeDecayPerSecond)가 겨룬다.
	// 순증가면 저절로 차고, 순감소면 깎인다(연타를 멈추면 AI가 되끌어내리도록). 좌우 전환의 능동 힘은 AddStruggleInput이 별도로 더한다.
	if (bCaptured && !bEscaped)
	{
		const float NetPerSecond = StrugglePassiveGainPerSecond - EscapeDecayPerSecond;
		if (NetPerSecond != 0.0f)
		{
			SetStruggle(AccumulatedStruggle + NetPerSecond * DeltaTime);
		}
	}
}

bool UFTCaptureEscapeComponent::TryBeginCapture(AActor* InCaptor, USceneComponent* InAttachPoint, float InEscapeThreshold, float InDecayPerSecond)
{
	if (bCaptured || !IsValid(InCaptor))
	{
		return false;
	}

	bCaptured = true;
	bEscaped = false;
	AccumulatedStruggle = 0.0f;
	// 임계값(AI 붙잡는 힘). 0/음수면 즉시 탈출·0나눗셈이 되므로 최소값으로 가드.
	EscapeThreshold = FMath::Max(InEscapeThreshold, KINDA_SMALL_NUMBER);
	// AI 자연감소(탈출 저지력). 음수는 방지(감소 안 함 하한 0).
	EscapeDecayPerSecond = FMath::Max(InDecayPerSecond, 0.0f);
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

		// 캡처 중엔 몸(캡슐 yaw)이 컨트롤 회전을 따라 돌지 않게 끈다. 그래야 부착된 캡처 포즈(경비 CapturePoint 회전)를
		// 따르고, 플레이어가 이송 중 시점을 돌려도 몸이 제자리에서 빙빙 돌지 않는다(시점은 스프링암이 별도로 처리).
		bSavedUseControllerRotationYaw = OwnerCharacter->bUseControllerRotationYaw;
		OwnerCharacter->bUseControllerRotationYaw = false;

		// 붙잡은 지점(경비 CapturePoint)에 부착해 경비 이동을 따라가게 한다.
		if (InAttachPoint)
		{
			OwnerCharacter->AttachToComponent(InAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}

	// 이송 중 플레이어 스프링암(카메라 붐)의 충돌 프로브(ECC_Camera)가 "잡은 경비"의 몸에 걸려
	// 카메라를 몸속으로 당기는 문제를 막는다. 경비의 캡슐/메시를 카메라 채널에서만 잠시 Ignore로 바꾸고
	// (벽 등 다른 충돌 판정은 그대로 유지), EndCapture에서 원래 응답으로 복구한다.
	if (ACharacter* CaptorCharacter = Cast<ACharacter>(InCaptor))
	{
		if (UCapsuleComponent* CaptorCapsule = CaptorCharacter->GetCapsuleComponent())
		{
			SavedCaptorCapsuleCameraResponse = CaptorCapsule->GetCollisionResponseToChannel(ECC_Camera);
			CaptorCapsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		}
		if (USkeletalMeshComponent* CaptorMesh = CaptorCharacter->GetMesh())
		{
			SavedCaptorMeshCameraResponse = CaptorMesh->GetCollisionResponseToChannel(ECC_Camera);
			CaptorMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		}
		bCaptorCameraResponseSaved = true;
	}

	SetComponentTickEnabled(true);
	return true;
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

		// 캡처 중 꺼둔 몸 회전(컨트롤러 yaw 추종)을 원복한다.
		OwnerCharacter->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystem())
	{
		ASC->RemoveLooseGameplayTag(TAG_FT_State_Captured);
	}

	// 캡터의 카메라 채널 응답 원복(TryBeginCapture에서 Ignore로 바꿔둔 경우에만). Captor가 이미 파괴됐으면 스킵.
	if (bCaptorCameraResponseSaved)
	{
		if (ACharacter* CaptorCharacter = Cast<ACharacter>(Captor.Get()))
		{
			if (UCapsuleComponent* CaptorCapsule = CaptorCharacter->GetCapsuleComponent())
			{
				CaptorCapsule->SetCollisionResponseToChannel(ECC_Camera, SavedCaptorCapsuleCameraResponse);
			}
			if (USkeletalMeshComponent* CaptorMesh = CaptorCharacter->GetMesh())
			{
				CaptorMesh->SetCollisionResponseToChannel(ECC_Camera, SavedCaptorMeshCameraResponse);
			}
		}
		bCaptorCameraResponseSaved = false;
	}

	AccumulatedStruggle = 0.0f;
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
			SetStruggle(AccumulatedStruggle + StruggleGainPerFlip);
		}
		LastStruggleSign = Sign;
	}
}

void UFTCaptureEscapeComponent::SetStruggle(float NewStruggle)
{
	// 누적 struggle을 [0, 임계값(AI 붙잡는 힘)] 범위로 클램프. 임계값에 도달하면 탈출.
	AccumulatedStruggle = FMath::Clamp(NewStruggle, 0.0f, EscapeThreshold);

	if (!bEscaped && AccumulatedStruggle >= EscapeThreshold)
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
