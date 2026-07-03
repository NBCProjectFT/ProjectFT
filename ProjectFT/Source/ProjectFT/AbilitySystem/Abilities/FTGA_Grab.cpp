// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_Grab.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "ProjectFT/AbilitySystem/Effects/FTGE_Damage.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Stun.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Components/FTCaptureEscapeComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Security/FTCaptureDestination.h"
#include "ProjectFT/Security/FTSecurityCharacter.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

UFTGA_Grab::UFTGA_Grab()
{
	// StateTree(또는 코드)가 Event.Grab으로 발동. 페이로드 Target = 잡을 대상(플레이어).
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_Grab;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);

	// 잡는 동안 경비에게 진행 태그 부여 — StateTree가 "아직 잡는 중?"을 이 태그로 감시한다(어빌리티 종료 시 자동 제거).
	ActivationOwnedTags.AddTag(TAG_FT_State_Grabbing);

	// 기본 GE(에디터에서 교체 가능).
	DamageEffectClass = UFTGE_Damage::StaticClass();
	StunEffectClass = UFTGE_Stun::StaticClass();
}

void UFTGA_Grab::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 인스턴스 재사용(InstancedPerActor) 대비 상태 리셋.
	bResolved = false;
	bBoundMoveCompleted = false;
	bCapturedMessageBroadcast = false;
	bEscapedMessageBroadcast = false;
	CapturedTarget = nullptr;
	TargetEscapeComp = nullptr;
	CachedAIController = nullptr;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	APawn* AvatarPawn = Cast<APawn>(Avatar);
	if (!AvatarPawn)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 대상: 이벤트 페이로드 우선, 없으면 0번 플레이어 폰(싱글).
	AActor* Target = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	if (!Target)
	{
		Target = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	UFTCaptureEscapeComponent* EscapeComp = Target ? Target->FindComponentByClass<UFTCaptureEscapeComponent>() : nullptr;

	// 확정 캐치 사거리 + 대상/컴포넌트 유효성 확인. 못 잡으면 취소 종료.
	if (!Target || !EscapeComp || EscapeComp->IsCaptured()
		|| FVector::Dist(Avatar->GetActorLocation(), Target->GetActorLocation()) > GrabRange)
	{
		UE_LOG(
			LogFTSecurity,
			Verbose,
			TEXT("Grab rejected: Security=%s Target=%s Captured=%s"),
			*GetNameSafe(Avatar),
			*GetNameSafe(Target),
			EscapeComp && EscapeComp->IsCaptured() ? TEXT("true") : TEXT("false")
		);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 부착 지점(경비 CapturePoint).
	USceneComponent* AttachPoint = nullptr;
	if (AFTSecurityCharacter* Security = Cast<AFTSecurityCharacter>(Avatar))
	{
		AttachPoint = Security->GetCapturePointComponent();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)
		|| !EscapeComp->TryBeginCapture(Avatar, AttachPoint))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CapturedTarget = Target;
	TargetEscapeComp = EscapeComp;

	// 탈출 성공 통지 바인딩 후 붙잡기 시작.
	EscapeComp->OnEscaped.AddDynamic(this, &UFTGA_Grab::OnTargetEscaped);
	BroadcastCaptureMessage(TAG_FT_Event_SecurityTargetCaptured);
	bCapturedMessageBroadcast = true;
	UE_LOG(LogFTSecurity, Log, TEXT("Security '%s' captured target '%s'"), *GetNameSafe(Avatar), *GetNameSafe(Target));

	// 이송: 가장 가까운 목적지로 MoveTo, 도달 시 실패. 없으면/컨트롤러 없으면 안전 타이머로 실패.
	AFTCaptureDestination* Destination = FindNearestCaptureDestination(Avatar->GetActorLocation());
	AAIController* AICon = Cast<AAIController>(AvatarPawn->GetController());
	CachedAIController = AICon;

	if (Destination && AICon)
	{
		bBoundMoveCompleted = true;
		AICon->ReceiveMoveCompleted.AddDynamic(this, &UFTGA_Grab::OnMoveCompleted);
		AICon->MoveToActor(Destination, Destination->AcceptanceRadius);
	}
	else
	{
		UE_LOG(LogFTNPC, Warning, TEXT("UFTGA_Grab: 이송 목적지/컨트롤러 없음 — %.1f초 폴백 타이머로 실패 처리."), FallbackCaptureSeconds);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(FallbackTimerHandle, this, &UFTGA_Grab::OnFallbackTimeout, FallbackCaptureSeconds, false);
		}
	}
}

void UFTGA_Grab::OnTargetEscaped()
{
	FinishGrab(/*bEscaped=*/true);
}

void UFTGA_Grab::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	// 목적지 도달(성공적 완료)만 실패 판정. 중단/무효는 무시(다른 이동 요청과 섞이는 것 방지).
	if (Result == EPathFollowingResult::Success)
	{
		FinishGrab(/*bEscaped=*/false);
	}
}

void UFTGA_Grab::OnFallbackTimeout()
{
	FinishGrab(/*bEscaped=*/false);
}

void UFTGA_Grab::FinishGrab(bool bEscaped)
{
	if (bResolved)
	{
		return;
	}
	bResolved = true;

	if (bEscaped)
	{
		// 성공 → 자신(경비)에게 스턴(SetByCaller로 지속시간 주입).
		if (StunEffectClass)
		{
			FGameplayEffectSpecHandle StunSpec = MakeOutgoingGameplayEffectSpec(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, StunEffectClass);
			if (StunSpec.IsValid())
			{
				StunSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_StunDuration, EscapeStunDuration);
				ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, StunSpec);
			}
		}

		BroadcastCaptureMessage(TAG_FT_Event_SecurityTargetEscaped);
		bEscapedMessageBroadcast = true;
	}
	else
	{
		// 실패 → 대상에게 데미지(SetByCaller, 음수 크기).
		if (DamageEffectClass && CapturedTarget.IsValid())
		{
			FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageEffectClass);
			if (DamageSpec.IsValid())
			{
				DamageSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Damage, -FailDamage);
				const FGameplayAbilityTargetDataHandle TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(CapturedTarget.Get());
				ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, TargetData);
			}
		}
	}

	// 공통 종료 → EndAbility에서 해방/이동정지/정리.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UFTGA_Grab::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 정리는 멱등적으로 — 외부 취소로 들어와도 안전하게 원복한다(FinishGrab이 이미 정리했으면 no-op).
	if (AAIController* AICon = CachedAIController.Get())
	{
		if (bBoundMoveCompleted)
		{
			AICon->ReceiveMoveCompleted.RemoveDynamic(this, &UFTGA_Grab::OnMoveCompleted);
			bBoundMoveCompleted = false;
		}
		AICon->StopMovement();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FallbackTimerHandle);
	}

	if (bWasCancelled && bCapturedMessageBroadcast && !bEscapedMessageBroadcast && CapturedTarget.IsValid())
	{
		BroadcastCaptureMessage(TAG_FT_Event_SecurityTargetEscaped);
		bEscapedMessageBroadcast = true;
	}

	if (UFTCaptureEscapeComponent* Comp = TargetEscapeComp.Get())
	{
		Comp->OnEscaped.RemoveDynamic(this, &UFTGA_Grab::OnTargetEscaped);
		Comp->EndCapture();
	}

	CapturedTarget = nullptr;
	TargetEscapeComp = nullptr;
	CachedAIController = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UFTGA_Grab::BroadcastCaptureMessage(FGameplayTag Channel) const
{
	AActor* SecurityActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = CapturedTarget.Get();
	if (!Channel.IsValid() || !SecurityActor || !TargetActor)
	{
		return;
	}

	FFTNPCReportPayloadStruct Payload;
	Payload.ReporterActor = SecurityActor;
	Payload.TargetActor = TargetActor;
	Payload.ReportLocation = TargetActor->GetActorLocation();
	Payload.ReportAmount = 0.0f;
	Payload.ReportProgress = 1.0f;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(Channel, Payload);
	UE_LOG(
		LogFTSecurity,
		Verbose,
		TEXT("Security capture message '%s': Reporter=%s Target=%s"),
		*Channel.ToString(),
		*GetNameSafe(SecurityActor),
		*GetNameSafe(TargetActor)
	);
}

AFTCaptureDestination* UFTGA_Grab::FindNearestCaptureDestination(const FVector& From) const
{
	AFTCaptureDestination* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<AFTCaptureDestination> It(World); It; ++It)
		{
			const float DistSq = FVector::DistSquared(From, It->GetActorLocation());
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = *It;
			}
		}
	}
	return Nearest;
}
