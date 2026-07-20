// Fill out your copyright notice in the Description page of Project Settings.

#include "FTInteractionComponent.h"

#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

#include "ProjectFT/AbilitySystem/FTPlayerAttributeSet.h"
#include "ProjectFT/Components/FTChanneledInteractionComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Interface/FTInteractable.h"

UFTInteractionComponent::UFTInteractionComponent()
{
	// 매 프레임 시야를 트레이스해 포커스 대상을 갱신한다(컴포넌트 자체 Tick — 소유 액터 Tick과 독립).
	PrimaryComponentTick.bCanEverTick = true;
}

void UFTInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 포커스 추적은 로컬 플레이어만 수행한다(포커스/프롬프트는 로컬 UI 관심사).
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	SetFocusedActor(TraceForInteractable());

	// 채널링 중이면: 스스로 끝났거나(완료/중단) 대상이 파괴/멀어지면 해제한다.
	// SetActiveChannel이 델리게이트 언바인드 + OnActiveChannelChanged 통지를 함께 처리한다.
	if (UFTChanneledInteractionComponent* Channel = ActiveChannel.Get())
	{
		if (!Channel->IsChanneling())
		{
			SetActiveChannel(nullptr);
		}
		else
		{
			const AActor* ChannelTarget = Channel->GetOwner();
			const float MaxDistSq = FMath::Square(InteractionDistance * 1.5f);
			const bool bTooFar = !ChannelTarget ||
				FVector::DistSquared(OwnerPawn->GetActorLocation(), ChannelTarget->GetActorLocation()) > MaxDistSq;
			if (bTooFar)
			{
				StopInteract();
			}
		}
	}
	else if (ActiveChannel.IsStale())
	{
		// 대상이 파괴됨(예: 훔치기 완료로 진열대 제거) → 정리 + UI에 종료 통지.
		SetActiveChannel(nullptr);
	}
}

void UFTInteractionComponent::TryInteract()
{
	AActor* Target = FocusedActor.Get();
	if (!Target)
	{
		return;
	}

	// 채널형(꾹 눌러 게이지) 대상이며 활성화 상태인 경우 채널링을 시작한다(즉시 상호작용은 하지 않음).
	if (UFTChanneledInteractionComponent* Channel = Target->FindComponentByClass<UFTChanneledInteractionComponent>())
	{
		if (Channel->IsActive())
		{
			SetActiveChannel(Channel);

			// 플레이어 손재주(Dexterity 속성)를 작업 속도 배수로 넘긴다(없으면 1.0 기본).
			float WorkSpeed = 1.0f;
			if (const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner()))
			{
				if (UAbilitySystemComponent* ASC = AbilityOwner->GetAbilitySystemComponent())
				{
					WorkSpeed = ASC->GetNumericAttribute(UFTPlayerAttributeSet::GetDexterityAttribute());
				}
			}

			Channel->StartChannel(GetOwner(), WorkSpeed);
			return;
		}
		else
		{
			UE_LOG(LogFTPlayer, Warning, TEXT("TryInteract: Channeled component on %s is NOT active!"), *GetNameSafe(Target));
		}
	}

	// 그 외엔 즉시 상호작용. BP/C++ 양쪽 구현을 위해 Execute_ 경로로 호출한다(직접 Cast 금지).
	if (Target->Implements<UFTInteractable>())
	{
		IFTInteractable::Execute_Interact(Target, GetOwner());
		UE_LOG(LogFTPlayer, Verbose, TEXT("Interact with '%s'."), *GetNameSafe(Target));
	}

	// 상호작용으로 대상이 파괴되거나 상태가 바뀌었을 수 있으니 포커스를 즉시 갱신한다.
	// (다음 Tick을 기다리지 않고 OnFocusedInteractableChanged가 바로 반영되도록)
	SetFocusedActor(TraceForInteractable());
}

void UFTInteractionComponent::StopInteract()
{
	if (UFTChanneledInteractionComponent* Channel = ActiveChannel.Get())
	{
		Channel->StopChannel();
	}
	SetActiveChannel(nullptr);
}

void UFTInteractionComponent::NotifySkillCheckInput()
{
	if (UFTChanneledInteractionComponent* Channel = ActiveChannel.Get())
	{
		Channel->NotifySkillCheckInput();
	}
}

AActor* UFTInteractionComponent::TraceForInteractable() const
{
	FVector ViewLocation;
	FVector ViewDirection;
	if (!GetViewPoint(ViewLocation, ViewDirection))
	{
		return nullptr;
	}

	const AActor* Owner = GetOwner();
	const FVector OwnerLocation = Owner ? Owner->GetActorLocation() : ViewLocation;

	// 3인칭: 카메라가 캐릭터 뒤에 있으므로, 카메라→몸 거리만큼 트레이스를 더 뻗어
	// '몸 기준' 도달 거리(InteractionDistance)를 확보한다. 카메라가 벽에 당겨져도(bDoCollisionTest) 자동 보정된다.
	const float CameraToOwner = FVector::Dist(ViewLocation, OwnerLocation);
	const FVector TraceEnd = ViewLocation + ViewDirection * (CameraToOwner + InteractionDistance);

	// 자기 자신(소유 폰)은 트레이스에서 제외한다.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FTInteractionTrace), false, GetOwner());

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, InteractionChannel, Params);

	// 조준선(카메라 ray)에 맞은 액터가 상호작용 가능하고, 그 지점이 '캐릭터 몸' 기준 도달 거리 안일 때만 포커스로 본다.
	// (카메라 기준이 아니라 몸 기준이라, 바라보기만 하고 멀리 떨어진 대상은 제외된다.)
	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	const bool bInteractable = HitActor
		&& (HitActor->Implements<UFTInteractable>() || HitActor->FindComponentByClass<UFTChanneledInteractionComponent>());
	const bool bInReach = bHit
		&& FVector::DistSquared(OwnerLocation, Hit.ImpactPoint) <= FMath::Square(InteractionDistance);
	const bool bAccepted = bInteractable && bInReach;

	if (bDebugDrawTrace)
	{
		DrawDebugLine(GetWorld(), ViewLocation, TraceEnd, bAccepted ? FColor::Green : FColor::Red, false, -1.0f, 0, 1.0f);
		// 몸 기준 도달 범위(InteractionDistance)를 구체로 표시한다.
		DrawDebugSphere(GetWorld(), OwnerLocation, InteractionDistance, 16, FColor::Cyan, false, -1.0f, 0, 1.0f);
	}

	return bAccepted ? HitActor : nullptr;
}

bool UFTInteractionComponent::GetViewPoint(FVector& OutLocation, FVector& OutDirection) const
{
	// 플레이어 시점(카메라)에서 트레이스한다. 컨트롤러의 뷰포인트가 실제 카메라 위치/회전을 반영한다.
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (const AController* Controller = OwnerPawn->GetController())
		{
			FRotator ViewRotation;
			Controller->GetPlayerViewPoint(OutLocation, ViewRotation);
			OutDirection = ViewRotation.Vector();
			return true;
		}
	}

	// 폴백: 컨트롤러가 없으면 액터의 눈 위치/전방을 사용한다.
	if (const AActor* Owner = GetOwner())
	{
		FRotator ViewRotation;
		Owner->GetActorEyesViewPoint(OutLocation, ViewRotation);
		OutDirection = ViewRotation.Vector();
		return true;
	}

	return false;
}

void UFTInteractionComponent::SetActiveChannel(UFTChanneledInteractionComponent* NewChannel)
{
	UFTChanneledInteractionComponent* Old = ActiveChannel.Get();
	if (Old == NewChannel && !ActiveChannel.IsStale())
	{
		return;
	}

	// 이전 채널의 스킬체크 델리게이트에서 언바인드(유효할 때만 — 파괴됐으면 이미 정리됨).
	if (Old)
	{
		Old->OnSkillCheckStarted.RemoveDynamic(this, &UFTInteractionComponent::HandleActiveSkillCheckStarted);
		Old->OnSkillCheckEnded.RemoveDynamic(this, &UFTInteractionComponent::HandleActiveSkillCheckEnded);
	}

	ActiveChannel = NewChannel;

	// 새 채널의 스킬체크 델리게이트에 바인드(여기 한곳에서만 관리하므로 UI는 신경 쓸 필요 없음).
	if (NewChannel)
	{
		NewChannel->OnSkillCheckStarted.AddDynamic(this, &UFTInteractionComponent::HandleActiveSkillCheckStarted);
		NewChannel->OnSkillCheckEnded.AddDynamic(this, &UFTInteractionComponent::HandleActiveSkillCheckEnded);
	}

	OnActiveChannelChanged.Broadcast(NewChannel ? NewChannel->GetOwner() : nullptr);
}

void UFTInteractionComponent::HandleActiveSkillCheckStarted()
{
	OnSkillCheckStarted.Broadcast();
}

void UFTInteractionComponent::HandleActiveSkillCheckEnded(EFTSkillCheckResultType Result)
{
	OnSkillCheckEnded.Broadcast(Result);
}

AActor* UFTInteractionComponent::GetActiveChannelActor() const
{
	UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel ? Channel->GetOwner() : nullptr;
}

bool UFTInteractionComponent::IsChanneling() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel != nullptr && Channel->IsChanneling();
}

float UFTInteractionComponent::GetChannelProgress() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel ? Channel->GetProgress() : 0.0f;
}

bool UFTInteractionComponent::IsSkillCheckActive() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel != nullptr && Channel->IsSkillCheckActive();
}

float UFTInteractionComponent::GetSkillCheckCursor() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel ? Channel->GetSkillCheckCursor() : 0.0f;
}

float UFTInteractionComponent::GetSkillCheckTarget() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel ? Channel->GetSkillCheckTarget() : 0.0f;
}

float UFTInteractionComponent::GetSkillCheckSuccessHalfWidth() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel ? Channel->GetSkillCheckSuccessHalfWidth() : 0.0f;
}

float UFTInteractionComponent::GetSkillCheckGreatHalfWidth() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel ? Channel->GetSkillCheckGreatHalfWidth() : 0.0f;
}

float UFTInteractionComponent::GetLastSkillCheckProgressBonus() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel ? Channel->GetLastSkillCheckProgressBonus() : 0.0f;
}

int32 UFTInteractionComponent::GetSkillCheckRewardSerial() const
{
	const UFTChanneledInteractionComponent* Channel = ActiveChannel.Get();
	return Channel ? Channel->GetSkillCheckRewardSerial() : 0;
}

void UFTInteractionComponent::SetFocusedActor(AActor* NewFocusedActor)
{
	// 주의: FocusedActor는 약참조라 대상이 Destroy되면 Get()이 null을 반환한다.
	// 이때 New도 null이면 "변화 없음"처럼 보이지만 실제로는 (대상)→null 전환이라 알려야 한다.
	// IsStale()로 "가리키던 대상이 파괴된" 경우를 구분해, 그 경우엔 early-return하지 않는다.
	if (FocusedActor.Get() == NewFocusedActor && !FocusedActor.IsStale())
	{
		return;
	}

	FocusedActor = NewFocusedActor;
	OnFocusedInteractableChanged.Broadcast(NewFocusedActor);
}
