// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_Grab.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "ProjectFT/AbilitySystem/Effects/FTGE_Damage.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Hostile.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Stun.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Components/FTCaptureEscapeComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Security/FTCaptureDestination.h"
#include "ProjectFT/Security/FTSecurityAIController.h"
#include "ProjectFT/Security/FTSecurityCharacter.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
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
	HostileMarkerEffectClass = UFTGE_Hostile::StaticClass();
}

void UFTGA_Grab::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 인스턴스 재사용(InstancedPerActor) 대비 상태 리셋.
	ResetGrabAttemptState();

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
	const float DistanceToTarget = Target ? FVector::Dist(Avatar->GetActorLocation(), Target->GetActorLocation()) : TNumericLimits<float>::Max();

	// StateTree/AIController가 시도 거리를 판단하고, 어빌리티는 잘못된 외부 호출만 안전 거리로 거른다.
	if (!Target || !EscapeComp || EscapeComp->IsCaptured()
		|| DistanceToTarget > GrabRange)
	{
		UE_LOG(
			LogFTSecurity,
			Log,
			TEXT("Grab rejected: Security=%s Target=%s Captured=%s Distance=%.1f SafetyRange=%.1f EscapeComp=%s"),
			*GetNameSafe(Avatar),
			*GetNameSafe(Target),
			EscapeComp && EscapeComp->IsCaptured() ? TEXT("true") : TEXT("false"),
			DistanceToTarget,
			GrabRange,
			EscapeComp ? TEXT("valid") : TEXT("null")
		);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 부착 지점/소켓(잡기 종류가 정하는 붙잡는 자세).
	AFTSecurityCharacter* Security = Cast<AFTSecurityCharacter>(Avatar);
	if (!Security)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FName AttachSocketName = NAME_None;
	USceneComponent* AttachPoint = ResolveCaptureAttachPoint(Security, AttachSocketName);

	if (!AttachPoint || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(
			LogFTSecurity,
			Log,
			TEXT("Grab rejected: Commit failed or attach point missing. Security=%s Target=%s AttachPoint=%s"),
			*GetNameSafe(Avatar),
			*GetNameSafe(Target),
			*GetNameSafe(AttachPoint));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AFTSecurityAIController* SecurityAIController = Cast<AFTSecurityAIController>(AvatarPawn->GetController()))
	{
		SecurityAIController->StartCaptureAttempt();
	}

	PendingTarget = Target;
	PendingEscapeComp = EscapeComp;
	PendingSecurity = Security;
	PendingAttachPoint = AttachPoint;
	PendingAttachSocketName = AttachSocketName;
	CreateGrabCaptureSphere();
	FaceTargetForGrab(AvatarPawn, Target);
	ApplyGrabMovementTuning(AvatarPawn);

	const float GrabMontageDuration = PlayGrabMontage();
	if (GrabMontageDuration <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GrabAttemptTimeoutTimerHandle,
			this,
			&UFTGA_Grab::OnGrabAttemptTimedOut,
			GrabMontageDuration + GrabAttemptTimeoutPadding,
			false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UFTGA_Grab::OpenGrabCaptureWindow()
{
	if (bResolved || bGrabCaptureConfirmed)
	{
		return;
	}

	bGrabCaptureWindowOpen = true;
	SetGrabCaptureSphereEnabled(true);
	DrawGrabCaptureDebugSphere();

	if (!bGrabCaptureConfirmed)
	{
		if (IsValid(GrabCaptureSphereComponent))
		{
			TArray<AActor*> OverlappingActors;
			GrabCaptureSphereComponent->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
			for (AActor* OverlappingActor : OverlappingActors)
			{
				TryConfirmCaptureFromActor(OverlappingActor);
				if (bGrabCaptureConfirmed)
				{
					break;
				}
			}
		}
	}
}

void UFTGA_Grab::CloseGrabCaptureWindow()
{
	if (!bGrabCaptureWindowOpen || bResolved || bGrabCaptureConfirmed)
	{
		return;
	}

	bGrabCaptureWindowOpen = false;
	SetGrabCaptureSphereEnabled(false);

	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Grab missed: capture window closed. Security=%s Target=%s Radius=%.1f"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*GetNameSafe(PendingTarget.Get()),
		GrabConfirmRadius);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UFTGA_Grab::OnGrabCaptureSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	TryConfirmCaptureFromActor(OtherActor);
}

void UFTGA_Grab::OnGrabAttemptTimedOut()
{
	if (bResolved || bGrabCaptureConfirmed)
	{
		return;
	}

	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Grab missed: capture notify window did not confirm. Security=%s Target=%s"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*GetNameSafe(PendingTarget.Get()));

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UFTGA_Grab::CreateGrabCaptureSphere()
{
	AFTSecurityCharacter* Security = PendingSecurity.Get();
	USkeletalMeshComponent* SecurityMesh = Security ? Security->GetMesh() : nullptr;
	if (!Security || !SecurityMesh || PendingAttachSocketName.IsNone() || !SecurityMesh->DoesSocketExist(PendingAttachSocketName))
	{
		return;
	}

	DestroyGrabCaptureSphere();

	GrabCaptureSphereComponent = NewObject<USphereComponent>(Security, TEXT("GrabCaptureSphere"));
	if (!GrabCaptureSphereComponent)
	{
		return;
	}

	GrabCaptureSphereComponent->SetSphereRadius(GrabConfirmRadius);
	GrabCaptureSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrabCaptureSphereComponent->SetCollisionObjectType(ECC_WorldDynamic);
	GrabCaptureSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	GrabCaptureSphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GrabCaptureSphereComponent->SetGenerateOverlapEvents(true);
	GrabCaptureSphereComponent->SetHiddenInGame(!bDrawGrabCaptureDebugSphere);
	GrabCaptureSphereComponent->SetVisibility(bDrawGrabCaptureDebugSphere);
	GrabCaptureSphereComponent->ShapeColor = FColor::Green;
	GrabCaptureSphereComponent->AttachToComponent(
		SecurityMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		PendingAttachSocketName);
	GrabCaptureSphereComponent->OnComponentBeginOverlap.AddDynamic(this, &UFTGA_Grab::OnGrabCaptureSphereBeginOverlap);
	GrabCaptureSphereComponent->RegisterComponent();
}

void UFTGA_Grab::DestroyGrabCaptureSphere()
{
	USphereComponent* CaptureSphere = GrabCaptureSphereComponent;
	if (!IsValid(CaptureSphere))
	{
		GrabCaptureSphereComponent = nullptr;
		return;
	}

	CaptureSphere->OnComponentBeginOverlap.RemoveDynamic(this, &UFTGA_Grab::OnGrabCaptureSphereBeginOverlap);
	CaptureSphere->DestroyComponent();
	GrabCaptureSphereComponent = nullptr;
}

void UFTGA_Grab::SetGrabCaptureSphereEnabled(bool bEnabled)
{
	USphereComponent* CaptureSphere = GrabCaptureSphereComponent;
	if (!IsValid(CaptureSphere))
	{
		GrabCaptureSphereComponent = nullptr;
		return;
	}

	CaptureSphere->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	CaptureSphere->SetHiddenInGame(!bDrawGrabCaptureDebugSphere);
	CaptureSphere->SetVisibility(bDrawGrabCaptureDebugSphere);

	if (bEnabled)
	{
		CaptureSphere->UpdateOverlaps();
	}
}

void UFTGA_Grab::TryConfirmCaptureFromActor(AActor* OtherActor)
{
	if (!bGrabCaptureWindowOpen || bResolved || bGrabCaptureConfirmed)
	{
		return;
	}

	AActor* Target = PendingTarget.Get();
	if (!OtherActor || OtherActor != Target)
	{
		return;
	}

	if (!PendingEscapeComp.IsValid())
	{
		return;
	}

	bGrabCaptureConfirmed = true;
	bGrabCaptureWindowOpen = false;
	SetGrabCaptureSphereEnabled(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GrabAttemptTimeoutTimerHandle);
	}

	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Grab confirmed by capture sphere overlap. Security=%s Target=%s Radius=%.1f"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*GetNameSafe(Target),
		GrabConfirmRadius);

	BeginConfirmedCapture();
}

void UFTGA_Grab::DrawGrabCaptureDebugSphere() const
{
	if (!bDrawGrabCaptureDebugSphere || !IsValid(GrabCaptureSphereComponent))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugSphere(
			World,
			GrabCaptureSphereComponent->GetComponentLocation(),
			GrabConfirmRadius,
			24,
			FColor::Green,
			false,
			GrabCaptureDebugSphereLifeTime,
			0,
			2.0f);
	}
}

void UFTGA_Grab::FaceTargetForGrab(APawn* AvatarPawn, const AActor* TargetActor) const
{
	if (!AvatarPawn || !TargetActor)
	{
		return;
	}

	FVector DirectionToTarget = TargetActor->GetActorLocation() - AvatarPawn->GetActorLocation();
	DirectionToTarget.Z = 0.0f;
	if (!DirectionToTarget.Normalize())
	{
		return;
	}

	const FRotator TargetYawRotation(0.0f, DirectionToTarget.Rotation().Yaw, 0.0f);
	AvatarPawn->SetActorRotation(TargetYawRotation);

	if (AController* Controller = AvatarPawn->GetController())
	{
		Controller->SetControlRotation(TargetYawRotation);
	}

	if (AAIController* AIController = Cast<AAIController>(AvatarPawn->GetController()))
	{
		AIController->SetFocus(const_cast<AActor*>(TargetActor), EAIFocusPriority::Gameplay);
	}
}

void UFTGA_Grab::BeginConfirmedCapture()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	AFTSecurityCharacter* Security = PendingSecurity.Get();
	AActor* Target = PendingTarget.Get();
	UFTCaptureEscapeComponent* EscapeComp = PendingEscapeComp.Get();
	USceneComponent* AttachPoint = PendingAttachPoint.Get();
	if (!Avatar || !Security || !Target || !EscapeComp || !AttachPoint || EscapeComp->IsCaptured())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 잡기 대칭 배타(B): 확정된 잡기 직전 대상에게 걸린 자가 행동불능을 제거한다.
	TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (UAbilitySystemComponent* TargetAbilitySystem = TargetASC.Get())
	{
		TargetAbilitySystem->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_FT_State_Debuff_Immobilized));
	}

	if (!EscapeComp->TryBeginCapture(Avatar, AttachPoint, PendingAttachSocketName, EscapeThreshold, EscapeDecayPerSecond))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	CapturedTarget = Target;
	TargetEscapeComp = EscapeComp;

	// 탈출 성공 통지 바인딩 후 붙잡기 시작.
	EscapeComp->OnEscaped.AddDynamic(this, &UFTGA_Grab::OnTargetEscaped);
	BroadcastCaptureMessage(TAG_FT_Event_SecurityTargetCaptured);
	bCapturedMessageBroadcast = true;

	// 붙잡힌 대가: 즉시 1회(InitialCaptureDamage) + 붙잡혀 있는 동안 초당 지속(CaptureDamagePerSecond).
	// 잡힌 것 자체의 대가이므로 이후 어떤 결말(탈출/이송/캡터 무력화)로 끝나든 되돌리지 않는다.
	// 공격 표식은 이 첫 1회에만 붙인다 — 잡히는 순간 피격음이 한 번 울리고, 지속 틱은 무음으로 체력만 깎는다.
	ApplyDamageToTarget(InitialCaptureDamage, /*bMarkHostile=*/true);
	StartCaptureDamageTick();

	if (UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo())
	{
		OwnerImmobilizedTagChangedHandle = OwnerASC->RegisterGameplayTagEvent(TAG_FT_State_Debuff_Immobilized, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UFTGA_Grab::OnOwnerImmobilizedTagChanged);

		if (OwnerASC->HasMatchingGameplayTag(TAG_FT_State_Debuff_Immobilized))
		{
			OnOwnerImmobilizedTagChanged(TAG_FT_State_Debuff_Immobilized, 1);
			return;
		}
	}
	UE_LOG(LogFTSecurity, Log, TEXT("Security '%s' captured target '%s'"), *GetNameSafe(Avatar), *GetNameSafe(Target));

	StartCaptureTransfer();
}

void UFTGA_Grab::ApplyGrabMovementTuning(APawn* AvatarPawn)
{
	if (bGrabMovementTuningApplied || !AvatarPawn)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(AvatarPawn);
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	TunedMovementComponent = Movement;
	SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
	SavedRotationRate = Movement->RotationRate;

	const float SafeSpeedScale = FMath::Max(0.0f, GrabMovementSpeedScale);
	const float SafeRotationScale = FMath::Max(0.0f, GrabRotationRateScale);
	Movement->MaxWalkSpeed = SavedMaxWalkSpeed * SafeSpeedScale;
	Movement->RotationRate = SavedRotationRate * SafeRotationScale;

	bGrabMovementTuningApplied = true;
}

void UFTGA_Grab::RestoreGrabMovementTuning()
{
	if (!bGrabMovementTuningApplied)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = TunedMovementComponent.Get())
	{
		Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
		Movement->RotationRate = SavedRotationRate;
	}

	TunedMovementComponent = nullptr;
	SavedMaxWalkSpeed = 0.0f;
	SavedRotationRate = FRotator::ZeroRotator;
	bGrabMovementTuningApplied = false;
}

void UFTGA_Grab::StartCaptureTransfer()
{
	if (bResolved || !CapturedTarget.IsValid())
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	APawn* AvatarPawn = Cast<APawn>(Avatar);
	if (!Avatar || !AvatarPawn)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 이송: 가장 가까운 목적지로 MoveTo, 도달 시 실패. 없으면/컨트롤러 없으면 안전 타이머로 실패.
	AFTCaptureDestination* Destination = FindNearestCaptureDestination(Avatar->GetActorLocation());
	AAIController* AICon = Cast<AAIController>(AvatarPawn->GetController());
	CachedAIController = AICon;

	if (Destination && AICon)
	{
		bBoundMoveCompleted = true;
		AICon->ReceiveMoveCompleted.AddDynamic(this, &UFTGA_Grab::OnMoveCompleted);
		const EPathFollowingRequestResult::Type MoveResult = AICon->MoveToActor(Destination, Destination->AcceptanceRadius);
		if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
		{
			CaptureTransferMoveRequestID = AICon->GetPathFollowingComponent()
				? AICon->GetPathFollowingComponent()->GetCurrentRequestId()
				: FAIRequestID::InvalidRequest;
			bWaitingForCaptureTransferMove = true;

			UE_LOG(
				LogFTSecurity,
				Log,
				TEXT("Grab transfer started. Security=%s Target=%s Destination=%s RequestID=%u Distance=%.1f AcceptanceRadius=%.1f"),
				*GetNameSafe(Avatar),
				*GetNameSafe(CapturedTarget.Get()),
				*GetNameSafe(Destination),
				CaptureTransferMoveRequestID.GetID(),
				FVector::Dist(Avatar->GetActorLocation(), Destination->GetActorLocation()),
				Destination->AcceptanceRadius);
		}
		else if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			UE_LOG(
				LogFTSecurity,
				Log,
				TEXT("Grab transfer completed immediately: already at destination. Security=%s Target=%s Destination=%s Distance=%.1f AcceptanceRadius=%.1f"),
				*GetNameSafe(Avatar),
				*GetNameSafe(CapturedTarget.Get()),
				*GetNameSafe(Destination),
				FVector::Dist(Avatar->GetActorLocation(), Destination->GetActorLocation()),
				Destination->AcceptanceRadius);
			FinishGrab(/*bEscaped=*/false);
		}
		else
		{
			UE_LOG(
				LogFTNPC,
				Warning,
				TEXT("UFTGA_Grab: 이송 MoveToActor 실패 — %.1f초 폴백 타이머로 실패 처리. Security=%s Target=%s Destination=%s"),
				FallbackCaptureSeconds,
				*GetNameSafe(Avatar),
				*GetNameSafe(CapturedTarget.Get()),
				*GetNameSafe(Destination));
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(FallbackTimerHandle, this, &UFTGA_Grab::OnFallbackTimeout, FallbackCaptureSeconds, false);
			}
		}
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
	if (!bWaitingForCaptureTransferMove || RequestID != CaptureTransferMoveRequestID)
	{
		UE_LOG(
			LogFTSecurity,
			Verbose,
			TEXT("Grab transfer ignored unrelated MoveCompleted. Expected=%u Received=%u Result=%d"),
			CaptureTransferMoveRequestID.GetID(),
			RequestID.GetID(),
			static_cast<int32>(Result));
		return;
	}

	bWaitingForCaptureTransferMove = false;
	CaptureTransferMoveRequestID = FAIRequestID::InvalidRequest;

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

void UFTGA_Grab::OnOwnerImmobilizedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0 || bResolved || !CapturedTarget.IsValid())
	{
		return;
	}

	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Security '%s' released captured target '%s' because captor became immobilized"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*GetNameSafe(CapturedTarget.Get())
	);

	FinishGrab(/*bEscaped=*/true);
}

void UFTGA_Grab::StartCaptureDamageTick()
{
	UWorld* World = GetWorld();
	if (!World || CaptureDamagePerSecond <= 0.0f)
	{
		return;
	}

	LastCaptureDamageTickTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(
		CaptureDamageTimerHandle,
		this,
		&UFTGA_Grab::TickCaptureDamage,
		CaptureDamageTickInterval,
		/*bLoop=*/true);
}

void UFTGA_Grab::TickCaptureDamage()
{
	UWorld* World = GetWorld();
	if (!World || bResolved)
	{
		return;
	}

	// 타이머 간격이 아니라 '실제 경과시간'으로 계산한다 — 프레임이 밀려 틱이 늦게 와도 초당 피해량이 그대로 보존된다.
	const float Now = World->GetTimeSeconds();
	const float DeltaSeconds = LastCaptureDamageTickTime > 0.0f
		? FMath::Max(0.0f, Now - LastCaptureDamageTickTime)
		: CaptureDamageTickInterval;
	LastCaptureDamageTickTime = Now;

	// 이미 쓰러진 대상은 더 때리지 않는다. 체력이 0에서 클램프돼도 피해 GE는 계속 '적중'으로 처리돼
	// Event.Character.Damaged/KnockedOut이 틱마다 재방송되기 때문(사망 처리 자체는 bDead 가드로 1회지만 메시지는 아니다).
	if (const UAbilitySystemComponent* TargetAbilitySystem = TargetASC.Get())
	{
		if (TargetAbilitySystem->HasMatchingGameplayTag(TAG_FT_State_Dead))
		{
			World->GetTimerManager().ClearTimer(CaptureDamageTimerHandle);
			return;
		}
	}

	ApplyDamageToTarget(CaptureDamagePerSecond * DeltaSeconds);
}

void UFTGA_Grab::ApplyDamageToTarget(float DamageAmount, bool bMarkHostile)
{
	if (DamageAmount <= 0.0f || !DamageEffectClass || !CapturedTarget.IsValid())
	{
		return;
	}

	FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageEffectClass);
	if (!DamageSpec.IsValid())
	{
		return;
	}

	// UFTGE_Damage는 SetByCaller(Data.Damage)를 Health에 그대로 더하는 Instant GE — 피해는 음수로 넣는다.
	DamageSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Damage, -DamageAmount);
	const FGameplayAbilityTargetDataHandle TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(CapturedTarget.Get());
	ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, TargetData);

	if (!bMarkHostile || !HostileMarkerEffectClass)
	{
		return;
	}

	// 공격 표식(효과 없는 마커 GE)은 피해 '뒤에' 적용한다 — 대상의 피격 훅이 깨어날 땐 체력이 이미 깎여 있어야
	// 연출/어그로가 실제 피해와 어긋나지 않는다.
	FGameplayEffectSpecHandle HostileSpec = MakeOutgoingGameplayEffectSpec(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, HostileMarkerEffectClass);
	if (HostileSpec.IsValid())
	{
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, HostileSpec, TargetData);
	}
}

USceneComponent* UFTGA_Grab::ResolveCaptureAttachPoint(AFTSecurityCharacter* Security, FName& OutAttachSocketName) const
{
	OutAttachSocketName = NAME_None;
	if (!Security)
	{
		return nullptr;
	}

	if (!CaptureAttachSocketName.IsNone())
	{
		// DoesSocketExist는 소켓과 본 이름을 모두 본다 — 소켓을 따로 안 만들고 본 이름을 바로 써도 된다.
		USkeletalMeshComponent* SecurityMesh = Security->GetMesh();
		if (SecurityMesh && SecurityMesh->DoesSocketExist(CaptureAttachSocketName))
		{
			OutAttachSocketName = CaptureAttachSocketName;
			return SecurityMesh;
		}

		UE_LOG(
			LogFTSecurity,
			Warning,
			TEXT("Grab: attach socket '%s' not found on %s — falling back to CapturePoint."),
			*CaptureAttachSocketName.ToString(),
			*GetNameSafe(SecurityMesh)
		);
	}

	// 소켓 미지정/부재 → 루트 기준 CapturePoint(소켓 도입 전 동작).
	return Security->GetCapturePointComponent();
}

float UFTGA_Grab::PlayGrabMontage() const
{
	UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance || !GrabMontage)
	{
		return 0.0f;
	}

	const float SafePlayRate = FMath::Max(GrabMontagePlayRate, KINDA_SMALL_NUMBER);
	const float PlayedDuration = AnimInstance->Montage_Play(GrabMontage, SafePlayRate);
	return PlayedDuration > 0.0f
		? PlayedDuration
		: GrabMontage->GetPlayLength() / SafePlayRate;
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
		// 실패 → 대상에게 데미지 + 레이드 실패 요청. 체력이 남아도 목적지 도착은 실패 조건이다.
		ApplyDamageToTarget(FailDamage);

		FFTMessagePayloadStruct Payload;
		Payload.InstigatorActor = GetAvatarActorFromActorInfo();
		Payload.TargetActor = CapturedTarget.Get();
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Request_Flow_FailRaid, Payload);
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
		World->GetTimerManager().ClearTimer(GrabAttemptTimeoutTimerHandle);
		World->GetTimerManager().ClearTimer(FallbackTimerHandle);
		World->GetTimerManager().ClearTimer(CaptureDamageTimerHandle);
	}

	if (OwnerImmobilizedTagChangedHandle.IsValid())
	{
		if (UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo())
		{
			OwnerASC->RegisterGameplayTagEvent(TAG_FT_State_Debuff_Immobilized, EGameplayTagEventType::NewOrRemoved)
				.Remove(OwnerImmobilizedTagChangedHandle);
		}
		OwnerImmobilizedTagChangedHandle.Reset();
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

	RestoreGrabMovementTuning();
	DestroyGrabCaptureSphere();

	CapturedTarget = nullptr;
	TargetEscapeComp = nullptr;
	TargetASC = nullptr;
	CachedAIController = nullptr;
	PendingTarget = nullptr;
	PendingEscapeComp = nullptr;
	PendingSecurity = nullptr;
	PendingAttachPoint = nullptr;
	PendingAttachSocketName = NAME_None;
	CaptureTransferMoveRequestID = FAIRequestID::InvalidRequest;
	bWaitingForCaptureTransferMove = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UFTGA_Grab::ResetGrabAttemptState()
{
	RestoreGrabMovementTuning();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GrabAttemptTimeoutTimerHandle);
		World->GetTimerManager().ClearTimer(FallbackTimerHandle);
		World->GetTimerManager().ClearTimer(CaptureDamageTimerHandle);
	}

	bResolved = false;
	bBoundMoveCompleted = false;
	bWaitingForCaptureTransferMove = false;
	bGrabCaptureWindowOpen = false;
	bGrabCaptureConfirmed = false;
	bCapturedMessageBroadcast = false;
	bEscapedMessageBroadcast = false;

	PendingTarget = nullptr;
	PendingEscapeComp = nullptr;
	PendingSecurity = nullptr;
	PendingAttachPoint = nullptr;
	PendingAttachSocketName = NAME_None;

	CapturedTarget = nullptr;
	TargetEscapeComp = nullptr;
	TargetASC = nullptr;
	CachedAIController = nullptr;
	DestroyGrabCaptureSphere();
	LastCaptureDamageTickTime = 0.0f;
	CaptureTransferMoveRequestID = FAIRequestID::InvalidRequest;
	TunedMovementComponent = nullptr;
	SavedMaxWalkSpeed = 0.0f;
	SavedRotationRate = FRotator::ZeroRotator;
	bGrabMovementTuningApplied = false;
	OwnerImmobilizedTagChangedHandle.Reset();
}

void UFTGA_Grab::BroadcastCaptureMessage(FGameplayTag Channel) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown)
	{
		return;
	}
	
	AActor* SecurityActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = CapturedTarget.Get();
	if (!Channel.IsValid() || !IsValid(SecurityActor) || !IsValid(TargetActor))
	{
		return;
	}

	FFTNPCReportPayloadStruct Payload;
	Payload.ReporterActor = SecurityActor;
	Payload.TargetActor = TargetActor;
	Payload.ReportLocation = TargetActor->GetActorLocation();
	Payload.ReportAmount = 0.0f;
	Payload.ReportProgress = 1.0f;
	
	UGameplayMessageSubsystem::Get(SecurityActor).BroadcastMessage(Channel, Payload);
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
