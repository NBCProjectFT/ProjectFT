#include "FTNPCReactionComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "ProjectFT/NPC/FTNPCAIController.h"
#include "ProjectFT/NPC/FTNPCCharacter.h"

namespace
{
	constexpr float NPCFleeDistance = 1200.0f;
	constexpr float NPCFleeFallbackDistance = 800.0f;
	constexpr float NPCFleeFallbackAngle = 45.0f;
	const FVector NPCFleeProjectionExtent(500.0f, 500.0f, 300.0f);
}

UFTNPCReactionComponent::UFTNPCReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTNPCReactionComponent::SetLastThreatActor(AActor* ThreatActor)
{
	LastThreatActor = ThreatActor;
}

void UFTNPCReactionComponent::EnterPanic()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	AActor* PanicTargetActor = LastThreatActor;

	if (!IsPlayerActor(PanicTargetActor))
	{
		return;
	}

	LastThreatActor = PanicTargetActor;
	Controller->TargetActor = PanicTargetActor;
	Controller->bPanicRequested = true;
	Controller->bFleeRequested = false;
	Controller->ClearReactionFocusState();
	PlaySurprisedMontage();

	// 공포 상태에서는 도망치기 전 자신을 위협한 플레이어를 잠깐 바라본다.
	Controller->SetFocus(PanicTargetActor, EAIFocusPriority::Gameplay);
}

void UFTNPCReactionComponent::HandleImmobilizedStateChanged(bool bImmobilized)
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	Controller->bIsStunned = bImmobilized;

	if (bImmobilized)
	{
		// 행동불능 상태가 된 적이 있으면 해제 시 공포 상태로 진입한다.
		bPanicAfterImmobilized = true;
		return;
	}

	if (bPanicAfterImmobilized && !Controller->bKnockedOut)
	{
		// 행동불능이 해제되면 즉시 도망치지 않고 공포 상태로 진입시킨다.
		bPanicAfterImmobilized = false;
		EnterPanic();
	}
}

bool UFTNPCReactionComponent::PickFleeLocationFrom(AActor* ThreatActor)
{
	const AFTNPCAIController* Controller = GetNPCAIController();
	APawn* ControlledPawn = Controller ? Controller->GetPawn() : nullptr;
	if (!ControlledPawn || !ThreatActor)
	{
		return false;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	FVector FleeDirection = PawnLocation - ThreatActor->GetActorLocation();
	FleeDirection.Z = 0.0f;

	if (FleeDirection.IsNearlyZero())
	{
		// 위치가 겹친 경우 현재 바라보는 반대 방향을 임시 도망 방향으로 사용한다.
		FleeDirection = -ControlledPawn->GetActorForwardVector();
		FleeDirection.Z = 0.0f;
	}

	FleeDirection.Normalize();

	const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavigationSystem)
	{
		return false;
	}

	const TArray<FVector> FleeDirections =
	{
		FleeDirection,
		FleeDirection.RotateAngleAxis(NPCFleeFallbackAngle, FVector::UpVector),
		FleeDirection.RotateAngleAxis(-NPCFleeFallbackAngle, FVector::UpVector)
	};
	const TArray<float> FleeDistances =
	{
		NPCFleeDistance,
		NPCFleeFallbackDistance
	};

	for (const FVector& CandidateDirection : FleeDirections)
	{
		for (const float CandidateDistance : FleeDistances)
		{
			// 기본 도망 지점이 NavMesh 밖이면 가까운 거리나 좌우 후보 위치를 다시 시도한다.
			const FVector DesiredFleeLocation = PawnLocation + CandidateDirection * CandidateDistance;
			FNavLocation ProjectedFleeLocation;
			if (NavigationSystem->ProjectPointToNavigation(DesiredFleeLocation, ProjectedFleeLocation, NPCFleeProjectionExtent))
			{
				AFTNPCAIController* MutableController = GetNPCAIController();
				if (MutableController)
				{
					MutableController->FleeLocation = ProjectedFleeLocation.Location;
				}
				return true;
			}
		}
	}

	return false;
}

bool UFTNPCReactionComponent::RequestFleeFromTarget()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return false;
	}

	AActor* ThreatActor = LastThreatActor.Get();
	if (!ThreatActor || Controller->bKnockedOut)
	{
		return false;
	}

	// 공포 상태가 끝난 뒤 마지막 위협 대상을 기준으로 실제 도망 상태를 요청한다.
	Controller->TargetActor = ThreatActor;
	Controller->bFleeRequested = PickFleeLocationFrom(ThreatActor);
	if (Controller->bFleeRequested)
	{
		Controller->bPanicRequested = false;
		StartFleeMovement();
	}

	return Controller->bFleeRequested;
}

void UFTNPCReactionComponent::FinishFlee()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	// 도망 상태가 끝나면 Shopping 상태로 정상 복귀할 수 있도록 요청 플래그를 정리한다.
	Controller->bFleeRequested = false;
	Controller->bPanicRequested = false;
	LastThreatActor = nullptr;
	Controller->FleeLocation = FVector::ZeroVector;

	StopFleeMovement();
}

void UFTNPCReactionComponent::TickReaction()
{
	const AFTNPCAIController* Controller = GetNPCAIController();
	if (Controller && Controller->bFleeRequested && !Controller->bKnockedOut)
	{
		StartFleeMovement();
	}
}

bool UFTNPCReactionComponent::PlaySurprisedMontage()
{
	return PlayRandomReactionAnimation(SurprisedAnimations);
}

bool UFTNPCReactionComponent::PlayReactingMontage()
{
	return PlayRandomReactionAnimation(ReactingAnimations);
}

AFTNPCAIController* UFTNPCReactionComponent::GetNPCAIController() const
{
	return Cast<AFTNPCAIController>(GetOwner());
}

bool UFTNPCReactionComponent::IsPlayerActor(const AActor* Actor) const
{
	const APawn* TargetPawn = Cast<APawn>(Actor);
	return TargetPawn && TargetPawn->IsPlayerControlled();
}

void UFTNPCReactionComponent::StartFleeMovement()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	// 도망 중에는 쇼핑 지점이나 신고 대상 응시가 남아 있으면 이동 방향과 시선이 어긋난다.
	Controller->ClearReactionFocusState();
	Controller->ClearFocus(EAIFocusPriority::Gameplay);
	Controller->SetFocalPoint(Controller->FleeLocation, EAIFocusPriority::Gameplay);

	if (AFTNPCCharacter* NPCCharacter = Cast<AFTNPCCharacter>(Controller->GetPawn()))
	{
		// 도망 상태에서는 쇼핑 이동보다 빠르게 이동하고 도망 애니메이션 조건을 켠다.
		if (!bHasPreFleeMoveSpeed)
		{
			PreFleeMoveSpeed = NPCCharacter->GetCharacterMovement()
				? NPCCharacter->GetCharacterMovement()->MaxWalkSpeed
				: PreFleeMoveSpeed;
			bHasPreFleeMoveSpeed = true;
		}
		NPCCharacter->SetMoveSpeed(Controller->FleeMoveSpeed);
		NPCCharacter->SetIsFleeing(true);
	}
}

void UFTNPCReactionComponent::StopFleeMovement()
{
	const AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	if (AFTNPCCharacter* NPCCharacter = Cast<AFTNPCCharacter>(Controller->GetPawn()))
	{
		// 쇼핑 상태로 돌아갈 때는 도망 전 이동 속도와 일반 보행 애니메이션으로 복구한다.
		if (bHasPreFleeMoveSpeed)
		{
			NPCCharacter->SetMoveSpeed(PreFleeMoveSpeed);
		}
		NPCCharacter->SetIsFleeing(false);
	}

	bHasPreFleeMoveSpeed = false;
	PreFleeMoveSpeed = 0.0f;
}

bool UFTNPCReactionComponent::PlayRandomReactionAnimation(const TArray<TObjectPtr<UAnimSequenceBase>>& Animations)
{
	if (Animations.IsEmpty())
	{
		return false;
	}

	const AFTNPCAIController* Controller = GetNPCAIController();
	ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	TArray<UAnimSequenceBase*> ValidAnimations;
	ValidAnimations.Reserve(Animations.Num());
	for (UAnimSequenceBase* Animation : Animations)
	{
		if (Animation)
		{
			ValidAnimations.Add(Animation);
		}
	}

	if (ValidAnimations.IsEmpty())
	{
		return false;
	}

	UAnimSequenceBase* SelectedAnimation = ValidAnimations[FMath::RandRange(0, ValidAnimations.Num() - 1)];
	return AnimInstance->PlaySlotAnimationAsDynamicMontage(
		SelectedAnimation,
		ReactionSlotName,
		0.1f,
		0.1f,
		ReactionAnimationPlayRate
	) != nullptr;
}
