// Fill out your copyright notice in the Description page of Project Settings.

#include "FTCaptureEscapeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Character/FTCharacterBase.h"

UFTCaptureEscapeComponent::UFTCaptureEscapeComponent()
{
	// 붙잡힌 동안에만 틱한다(게이지 힘싸움 = 수동증가 vs 저지력).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 잡기 기본값 보존: 플레이어 자연증가 1.0/s(연타 안 해도 조금씩 차오르며 AI 저지력과 힘싸움).
	// 비눗방울(순수 연타, Passive=0)과 달리 잡기는 이 값을 기본으로 갖는다.
	Gauge.PassiveGainPerSecond = 1.0f;
}

void UFTCaptureEscapeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 힘싸움: 매 틱 플레이어 자연증가(Gauge.PassiveGainPerSecond)와 AI 자연감소(주입된 DecayPerSecond)가 겨룬다.
	// 좌우 전환의 능동 힘은 OnStruggleEvent(Event.Struggle)가 별도로 더한다.
	// 소유자가 죽으면 게이지는 그 값에서 얼어붙는다 — 시체는 발버둥치지도, 저지당하지도 않는다.
	if (bCaptured && !bEscaped && !IsOwnerDead())
	{
		Gauge.Advance(DeltaTime);
		TryComplete();
	}
}

void UFTCaptureEscapeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 붙잡힌 채로 파괴돼도 태그/리스너가 남지 않도록 정리.
	if (bCaptured)
	{
		EndCapture();
	}
	Super::EndPlay(EndPlayReason);
}

bool UFTCaptureEscapeComponent::TryBeginCapture(AActor* InCaptor, USceneComponent* InAttachPoint, FName InAttachSocketName, float InEscapeThreshold, float InDecayPerSecond)
{
	if (bCaptured || !IsValid(InCaptor))
	{
		return false;
	}

	bCaptured = true;
	bEscaped = false;
	// 공용 게이지 시작: 임계값(AI 붙잡는 힘)·저지력(AI 탈출 저지력) 주입 + 누적 리셋.
	Gauge.Begin(InEscapeThreshold, InDecayPerSecond);
	Captor = InCaptor;

	// 붙잡힘 관계 상태 + 공통 행동불능 태그. 탈출 게이지는 이 컴포넌트가 직접 Event.Struggle을 수신해 처리한다.
	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystem())
	{
		ASC->AddLooseGameplayTag(TAG_FT_State_Captured);
		ASC->AddLooseGameplayTag(TAG_FT_State_Debuff_Immobilized);
		StruggleEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(TAG_FT_Event_Struggle)
			.AddUObject(this, &UFTCaptureEscapeComponent::OnStruggleEvent);
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

		// 캡처 중엔 몸(캡슐 yaw)이 컨트롤 회전을 따라 돌지 않게 끈다. 그래야 부착된 캡처 포즈(부착 지점의 회전)를 따른다.
		bSavedUseControllerRotationYaw = OwnerCharacter->bUseControllerRotationYaw;
		OwnerCharacter->bUseControllerRotationYaw = false;

		// 어빌리티가 정한 지점/소켓에 부착해 경비를 따라가게 한다. 소켓이 지정되면 그 본의 애니메이션까지 따라간다.
		// 붙는 건 소유자의 루트(캡슐)이므로 소켓 트랜스폼 = 캡슐 중심이 놓일 자리다(발밑이 아니다).
		if (InAttachPoint)
		{
			OwnerCharacter->AttachToComponent(InAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale, InAttachSocketName);
		}
	}

	// 이송 중 플레이어 스프링암(카메라 붐)의 충돌 프로브(ECC_Camera)가 "잡은 경비"의 몸에 걸려 카메라를 몸속으로
	// 당기는 문제를 막는다. 경비의 캡슐/메시를 카메라 채널에서만 잠시 Ignore로 바꾸고, EndCapture에서 복구한다.
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

		// 캡슐을 다시 수직으로 세운다(yaw만 보존). 부착 중엔 캡슐이 부착 지점의 회전을 그대로 따르는데, 소켓이
		// 애니메이션되는 본이면 pitch/roll까지 물려받고 KeepWorldTransform으로 떼면 그 기울기가 남는다.
		// 캐릭터는 bUseControllerRotationPitch/Roll이 false라 yaw만 컨트롤러 추종으로 복구될 뿐 pitch/roll을 되돌리는
		// 주체가 없어서, 세워주지 않으면 기울어진 채로 영영 굳는다(루트에 붙던 시절엔 부착 지점이 늘 수직이라 안 드러났다).
		FRotator UprightRotation = OwnerCharacter->GetActorRotation();
		UprightRotation.Pitch = 0.0f;
		UprightRotation.Roll = 0.0f;
		OwnerCharacter->SetActorRotation(UprightRotation);

		if (UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(SavedCollisionEnabled);
		}

		// 캡처 중 꺼둔 몸 회전(컨트롤러 yaw 추종)을 원복한다.
		OwnerCharacter->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystem())
	{
		ASC->RemoveLooseGameplayTag(TAG_FT_State_Captured);
		ASC->RemoveLooseGameplayTag(TAG_FT_State_Debuff_Immobilized);
		if (StruggleEventHandle.IsValid())
		{
			if (FGameplayEventMulticastDelegate* Delegate = ASC->GenericGameplayEventCallbacks.Find(TAG_FT_Event_Struggle))
			{
				Delegate->Remove(StruggleEventHandle);
			}
			StruggleEventHandle.Reset();
		}
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

	Gauge.Reset();
	Captor = nullptr;
}

void UFTCaptureEscapeComponent::OnStruggleEvent(const FGameplayEventData* Payload)
{
	if (!bCaptured || bEscaped || IsOwnerDead())
	{
		return;
	}

	// Event.Struggle 1발 = 좌우 전환 1회(능동 탈출력). flip 판정은 입력측(플레이어)이 이미 했다.
	Gauge.AddFlip();
	if (AFTCharacterBase* OwnerCharacter = Cast<AFTCharacterBase>(GetOwner()))
	{
		OwnerCharacter->PlayStruggleJitter();
	}
	TryComplete();
}

void UFTCaptureEscapeComponent::TryComplete()
{
	if (!bEscaped && Gauge.IsFull())
	{
		bEscaped = true;
		// 어빌리티가 이걸 받아 성공 처리(해방 + 경비 스턴)한다. EndCapture는 어빌리티가 호출한다.
		OnEscaped.Broadcast();
	}
}

bool UFTCaptureEscapeComponent::IsOwnerDead() const
{
	// State.Dead는 사망 질의의 단일 소스(AFTCharacterBase::HandleDeath가 Loose 태그로 부여).
	const UAbilitySystemComponent* ASC = GetOwnerAbilitySystem();
	return ASC && ASC->HasMatchingGameplayTag(TAG_FT_State_Dead);
}

UAbilitySystemComponent* UFTCaptureEscapeComponent::GetOwnerAbilitySystem() const
{
	if (const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return AbilityOwner->GetAbilitySystemComponent();
	}
	return nullptr;
}
