
#include "FTSecurityAIController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Components/FTSecurityCaptureStateComponent.h"
#include "ProjectFT/Components/FTSecurityResponseComponent.h"
#include "ProjectFT/Components/FTSecurityReturnComponent.h"
#include "ProjectFT/Components/FTSecurityPursuitStateComponent.h"
#include "ProjectFT/Components/FTSecurityCallComponent.h"
#include "ProjectFT/Components/FTSecurityTargetComponent.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Struct/FTCharacterAttackedPayloadStruct.h"
#include "ProjectFT/Struct/FTSecurityChaseGaugePayloadStruct.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionTypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Core/FTGameState.h"
#include "ProjectFT/Components/FTSecurityCoordinationComponent.h"
#include "ProjectFT/Struct/FTSecurityResponsePayloadStruct.h"
#include "ProjectFT/Security/FTSecurityCharacter.h"

// 개발 테스트용 콘솔 명령어. 출시/제출 전 제거 대상.
static FAutoConsoleCommandWithWorld GFTSecurityTestCallCommand(
	TEXT("ft.Security.TestCall"),
	TEXT("Broadcasts Event.Security.Called with the first player pawn as TargetActor."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("Security AI TestCall: World is null"));
			return;
		}

		APawn* PlayerPawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
		if (!PlayerPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("Security AI TestCall: PlayerPawn is null"));
			return;
		}

		FFTNPCReportPayloadStruct Payload;
		Payload.TargetActor = PlayerPawn;
		Payload.ReportLocation = PlayerPawn->GetActorLocation();
		Payload.ReportAmount = 100.0f;
		Payload.ReportProgress = 1.0f;

		UGameplayMessageSubsystem::Get(World).BroadcastMessage(TAG_FT_Event_SecurityCalled, Payload);
		UE_LOG(LogTemp, Log, TEXT("Security AI TestCall: Broadcast Event.Security.Called for %s"), *PlayerPawn->GetName());
	})
);


AFTSecurityAIController::AFTSecurityAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SecurityStateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("SecurityStateTreeAIComponent"));
	SecurityStateTreeAIComponent->SetStartLogicAutomatically(false);
	
	SecurityPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("SecurityPerceptionComponent"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	ConfigureSight(SecurityPerceptionComponent, SightConfig, SecuritySightRadius, SecurityPeripheralVisionAngle, SecuritySightMaxAge);

	SecurityCallComponent = CreateDefaultSubobject<UFTSecurityCallComponent>(TEXT("SecurityCallComponent"));
	SecurityTargetComponent = CreateDefaultSubobject<UFTSecurityTargetComponent>(TEXT("SecurityTargetComponent"));
	SecurityResponseComponent = CreateDefaultSubobject<UFTSecurityResponseComponent>(TEXT("SecurityResponseComponent"));
	SecurityReturnComponent = CreateDefaultSubobject<UFTSecurityReturnComponent>(TEXT("SecurityReturnComponent"));
	SecurityCaptureStateComponent = CreateDefaultSubobject<UFTSecurityCaptureStateComponent>(TEXT("SecurityCaptureStateComponent"));
	SecurityPursuitStateComponent = CreateDefaultSubobject<UFTSecurityPursuitStateComponent>(TEXT("SecurityPursuitStateComponent"));
}

void AFTSecurityAIController::PreInitializeComponents()
{
	SecurityStateTreeAIComponent->SetStartLogicAutomatically(false);
	Super::PreInitializeComponents();
}

void AFTSecurityAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (SecurityReturnComponent)
	{
		SecurityReturnComponent->InitializeHome(this, InPawn);
	}

	if (SecurityStateTreeAIComponent)
	{
		SecurityStateTreeAIComponent->StartLogic();
	}
}

void AFTSecurityAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bKnockedOut)
	{
		return;
	}

	UpdateAbilityState();
	UpdateTargetState();
	UpdateReturnTargetMemory();
	UpdateTargetFocus();
	UpdateSecurityCallGauge(DeltaTime);
	if (SecurityReturnComponent)
	{
		SecurityReturnComponent->UpdateReturnCollision(this);
	}
	DrawSightDebug();
}

void AFTSecurityAIController::UpdateAbilityState()
{
	bIsGrabbing = false;
	bIsStunned = false;

	const IAbilitySystemInterface* AbilitySystemActor = Cast<IAbilitySystemInterface>(GetPawn());
	const UAbilitySystemComponent* ASC = AbilitySystemActor ? AbilitySystemActor->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	bIsGrabbing = ASC->HasMatchingGameplayTag(TAG_FT_State_Grabbing);
	// '멈춤' 인지는 스턴 하나가 아니라 행동불능 우산 태그로 판정한다(스턴/마비/비눗방울/빙결 전부 동일).
	// 안 그러면 버블 등으로 CMC만 정지될 때 AI 브레인이 자기가 묶인 걸 몰라 이동 실패 후 멈춘 채로 재개하지 못한다.
	// (StateTree는 이 플래그로 스턴처럼 정지→해제 시 재개하므로, 우산 태그로 넓히면 모든 행동불능이 동일하게 처리된다.)
	bIsStunned = ASC->HasMatchingGameplayTag(TAG_FT_State_Debuff_Immobilized);
	if (!bIsStunned)
	{
		bStunRequested = false;
	}
}

void AFTSecurityAIController::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		if (AFTGameState* GameState = World->GetGameState<AFTGameState>())
		{
			GameState->SecurityCoordinationComponent->RegisterSecurityController(this);
		}
	}

	ApplySecuritySightConfig();
	

	// Event.Security.Called 메시지가 발행될 때마다 OnSecurityCalled()가 호출됨.
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	SecurityCalledListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityCalled, 
		this, 
		&ThisClass::OnSecurityCalled
	);
	ChaseGaugeChangedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityChaseGaugeChanged,
		this,
		&ThisClass::OnChaseGaugeChanged
	);
	ChaseEndedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityChaseEnded,
		this,
		&ThisClass::OnChaseEnded
	);
	SecurityDeployedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityDeployed,
		this,
		&ThisClass::OnSecurityDeployed
	);
	SecurityTargetCapturedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityTargetCaptured,
		this,
		&ThisClass::OnSecurityTargetCaptured
	);
	SecurityTargetEscapedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityTargetEscaped,
		this,
		&ThisClass::OnSecurityTargetEscaped
	);
	ShelfDamagedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_ShelfDamaged,
		this,
		&ThisClass::OnShelfDamaged
	);
	CharacterAttackedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_CharacterAttacked,
		this,
		&ThisClass::OnCharacterAttacked
	);

	if (SecurityReturnComponent)
	{
		SecurityReturnComponent->InitializeHome(this, GetPawn());
	}
	
	if (SecurityPerceptionComponent)
	{
		SecurityPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AFTSecurityAIController::OnTargetPerceptionUpdated);
	}
	
}

void AFTSecurityAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !IsPlayerActor(Actor))
	{
		return;
	}

	if (bTargetCaptured)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		if (!TargetActor)
		{
			SetTargetActor(Actor);
		}

		if (!bSecurityCalled && Actor == TargetActor && IsTargetStealing(Actor))
		{
			bReturning = false;
			bSecurityCalled = true;
			bCanRequestSecuritySupport = true;
			if (SecurityCallComponent)
			{
				SecurityCallComponent->StartSecurityCall(Actor);
			}
			InvestigateLocation = Actor->GetActorLocation();
			if (bLogPerceptionDebug)
			{
				UE_LOG(LogFTSecurity, Log, TEXT("Security AI: Direct theft witnessed, chasing %s"), *GetNameSafe(Actor));
			}
		}

		if (Actor != TargetActor)
		{
			return;
		}

		UpdateTargetState();
		// MoveToActor(TargetActor, 150.0f);
		// UE_LOG(LogTemp, Log, TEXT("Security AI: Target sensed, chasing"));

		const APawn* ControlledPawn = GetPawn();
		if (!ControlledPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("Security AI: ControlledPawn is null"));
			return;
		}

		const float Distance = FVector::Dist(
			ControlledPawn->GetActorLocation(),
			TargetActor->GetActorLocation()
		);

		if (Distance <= 150.0f)
		{
			if (bLogPerceptionDebug)
			{
				UE_LOG(LogFTSecurity, Log, TEXT("Security AI: Attack range reached"));
			}
		}
	}
	else
	{
		if (Actor != TargetActor)
		{
			return;
		}

		UpdateTargetState();
		
		// 플레이어를 마지막으로 감지했던 위치를 조사 위치로 저장
		if (!Stimulus.StimulusLocation.IsNearlyZero())
		{
			InvestigateLocation = Stimulus.StimulusLocation;
		}
		// const FVector DetectedLocation = Stimulus.StimulusLocation;
		// MoveToLocation(DetectedLocation);
		if (bLogPerceptionDebug)
		{
			UE_LOG(LogFTSecurity, Log, TEXT("Security AI: Target lost, moving to last known location"));
		}
	}
}

void AFTSecurityAIController::SetTargetActor(AActor* NewTargetActor)
{
	if (TargetActor != NewTargetActor)
	{
		if (SecurityTargetComponent)
		{
			SecurityTargetComponent->ResetTargetMemory();
		}
	}

	TargetActor = NewTargetActor;
}

void AFTSecurityAIController::StartChase()
{
	if (!TargetActor || bTargetCaptured)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: TargetActor is null"));
		return;
	}

	constexpr float ChaseAcceptanceRadius = 75.0f;
	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(TargetActor, ChaseAcceptanceRadius);
	if (bLogSecurityEventDebug)
	{
		UE_LOG(LogFTSecurity, Log, TEXT("Security AI: MoveToActor result %d"), static_cast<int32>(MoveResult));
	}
}

void AFTSecurityAIController::ReadyDespawn()
{
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);

	if (SecurityPerceptionComponent)
	{
		SecurityPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AFTSecurityAIController::OnTargetPerceptionUpdated);
		SecurityPerceptionComponent->Deactivate();
	}

	if (SecurityStateTreeAIComponent && SecurityStateTreeAIComponent->IsComponentTickEnabled())
	{
		SecurityStateTreeAIComponent->StopLogic(TEXT("ReadyDespawn"));
	}

	SetActorTickEnabled(false);
}

void AFTSecurityAIController::HandleControlledPawnDeath()
{
	if (SecurityCallComponent)
	{
		SecurityCallComponent->StopSecurityCall();
	}

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);

	TargetActor = nullptr;
	bSecurityCalled = false;
	bHasSeenTarget = false;
	bIsTargetInAttackRange = false;
	bDetectedTargetByCloseRange = false;
	bHasObservedCrime = false;
	TargetDistance = 0.0f;
	bReturning = false;
	bRememberingTarget = false;
	TargetMemoryEndTime = 0.0f;
	bReacquiredTargetDuringReturn = false;
	SecurityChaseGauge = 0.0f;
	bSecurityChaseActive = false;
	bTargetCaptured = false;
	bIsCaptor = false;
	bIsTargetCapturedByOtherSecurity = false;
	bReturnRequested = false;
	bInvestigateRequested = false;
	bStunRequested = false;
	bIsGrabbing = false;
	bIsStunned = false;
	bKnockedOut = true;
	bIsAttackLeader = false;
	bParticipatingInChase = false;
	EncircleSlotLocation = FVector::ZeroVector;
	bHasEncircleSlot = false;
	bCanRequestSecuritySupport = false;

	if (AFTSecurityCharacter* SecurityCharacter = Cast<AFTSecurityCharacter>(GetPawn()))
	{
		SecurityCharacter->bIsIdle = false;
		SecurityCharacter->bIsObserving = false;
		SecurityCharacter->bIsRequestingSupport = false;
		SecurityCharacter->bIsApproachingTarget = false;
		SecurityCharacter->bIsGrabbing = false;
		SecurityCharacter->bIsExpelling = false;
		SecurityCharacter->bIsWaitingBeforeReturn = false;
		SecurityCharacter->bIsAttacking = false;
		SecurityCharacter->bIsTryingAttack = false;
		SecurityCharacter->bIsAttackDelay = false;
		SecurityCharacter->bIsAttackCooldown = false;
		SecurityCharacter->bIsKnockedOut = true;
	}

	if (SecurityPerceptionComponent)
	{
		SecurityPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &AFTSecurityAIController::OnTargetPerceptionUpdated);
		SecurityPerceptionComponent->Deactivate();
	}
}

void AFTSecurityAIController::FinishKnockedOut()
{
	if (AFTAICharacterBase* AICharacter = Cast<AFTAICharacterBase>(GetPawn()))
	{
		AICharacter->DespawnAfterDeath();
	}
}

void AFTSecurityAIController::OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	if (SecurityResponseComponent)
	{
		SecurityResponseComponent->HandleSecurityCalled(this, Payload);
	}
}

void AFTSecurityAIController::OnShelfDamaged(
	FGameplayTag Channel,
	const FFTMessagePayloadStruct& Payload)
{
	if (SecurityResponseComponent)
	{
		SecurityResponseComponent->HandleShelfDamaged(this, Payload);
	}
}

void AFTSecurityAIController::OnCharacterAttacked(FGameplayTag Channel, const FFTCharacterAttackedPayloadStruct& Payload)
{
	if (SecurityResponseComponent)
	{
		SecurityResponseComponent->HandleCharacterAttacked(this, Payload);
	}
}

AActor* AFTSecurityAIController::GetTargetActor() const
{
	return TargetActor;
}

bool AFTSecurityAIController::IsRememberingTarget() const
{
	return bRememberingTarget && TargetActor != nullptr;
}

bool AFTSecurityAIController::CanStartCaptureAttempt() const
{
	if (!TargetActor || bTargetCaptured || bIsGrabbing || bIsStunned || !bSecurityChaseActive)
	{
		return false;
	}

	if (!bIsTargetInAttackRange)
	{
		return false;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return CurrentTime - LastCaptureAttemptTime >= CaptureRetryCooldown;
}

void AFTSecurityAIController::StartCaptureAttempt()
{
	LastCaptureAttemptTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bCanStartCaptureAttempt = false;
}

void AFTSecurityAIController::TryStartImmediateCaptureAttempt()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !TargetActor || !bIsAttackLeader || !bCanStartCaptureAttempt)
	{
		return;
	}

	const IAbilitySystemInterface* AbilitySystemActor = Cast<IAbilitySystemInterface>(ControlledPawn);
	const UAbilitySystemComponent* ASC = AbilitySystemActor ? AbilitySystemActor->GetAbilitySystemComponent() : nullptr;
	if (!ASC || ASC->HasMatchingGameplayTag(TAG_FT_State_Grabbing))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = TAG_FT_Event_Grab;
	Payload.Instigator = ControlledPawn;
	Payload.Target = TargetActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ControlledPawn, TAG_FT_Event_Grab, Payload);
}

void AFTSecurityAIController::ApplySecuritySightConfig()
{
	if (!SecurityPerceptionComponent || !SightConfig)
	{
		return;
	}

	SightConfig->SightRadius = SecuritySightRadius;
	SightConfig->LoseSightRadius = FMath::Max(SecurityLoseSightRadius, SecuritySightRadius);
	SightConfig->PeripheralVisionAngleDegrees = SecurityPeripheralVisionAngle;
	SightConfig->SetMaxAge(SecuritySightMaxAge);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	SecurityPerceptionComponent->ConfigureSense(*SightConfig);
	SecurityPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*SecurityPerceptionComponent);
	RefreshSightConfig(SecurityPerceptionComponent, SightConfig);
}

void AFTSecurityAIController::UpdateTargetState()
{
	if (bTargetCaptured)
	{
		TargetDistance = 0.0f;
		bHasSeenTarget = false;
		bIsTargetInAttackRange = false;
		bDetectedTargetByCloseRange = false;
		bHasObservedCrime = false;
		bCanStartCaptureAttempt = false;
		UpdateChaseGaugeTargetSeenState();
		return;
	}

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		TargetDistance = 0.0f;
		bHasSeenTarget = false;
		bIsTargetInAttackRange = false;
		bDetectedTargetByCloseRange = false;
		bHasObservedCrime = false;
		bCanStartCaptureAttempt = false;
		if (SecurityTargetComponent)
		{
			SecurityTargetComponent->ResetTargetMemory();
		}
		UpdateChaseGaugeTargetSeenState();
		return;
	}

	if (!TargetActor)
	{
		if (AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (IsPlayerActor(PlayerActor) && IsActorDetectedByCloseRange(PlayerActor))
			{
				SetTargetActor(PlayerActor);
			}
		}
	}

	if (!TargetActor)
	{
		TargetDistance = 0.0f;
		bHasSeenTarget = false;
		bIsTargetInAttackRange = false;
		bDetectedTargetByCloseRange = false;
		bHasObservedCrime = false;
		bCanStartCaptureAttempt = false;
		if (SecurityTargetComponent)
		{
			SecurityTargetComponent->ResetTargetMemory();
		}
		UpdateChaseGaugeTargetSeenState();
		return;
	}

	bDetectedTargetByCloseRange = IsTargetDetectedByCloseRange();
	const bool bTargetStealing = IsTargetStealing(TargetActor);
	bHasObservedCrime = bTargetStealing || bSecurityCalled || bSecurityChaseActive;

	if (SecurityTargetComponent)
	{
		SecurityTargetComponent->UpdateTargetState(
			this,
			TargetActor,
			IsTargetCurrentlyVisible(),
			AttackRange,
			TargetSightLostGracePeriod,
			TargetDistance,
			bHasSeenTarget,
			bIsTargetInAttackRange);
	}

	if (!bReturning && !bSecurityCalled && bHasSeenTarget && bTargetStealing)
	{
		bReturning = false;
		bSecurityCalled = true;
		bHasObservedCrime = true;
		bCanRequestSecuritySupport = true;
		if (SecurityCallComponent)
		{
			SecurityCallComponent->StartSecurityCall(TargetActor);
		}
		InvestigateLocation = TargetActor->GetActorLocation();
		if (bLogPerceptionDebug)
		{
			UE_LOG(LogFTSecurity, Log, TEXT("Security AI: Direct theft witnessed, chasing %s"), *GetNameSafe(TargetActor));
		}
	}
	
	// 근접 감지는 즉시 추격이 아니라 StateTree의 경계/확인 상태로 넘기기 위한 위치만 갱신한다.
	if (bDetectedTargetByCloseRange && !bHasSeenTarget)
	{
		InvestigateLocation = TargetActor->GetActorLocation();
	}

	// 현재 보이는 상태라면 마지막 목격 위치 갱신
	if (bHasSeenTarget)
	{
		InvestigateLocation = TargetActor->GetActorLocation();
	}

	UpdateChaseGaugeTargetSeenState();
	bCanStartCaptureAttempt = CanStartCaptureAttempt();
	TryStartImmediateCaptureAttempt();
}

void AFTSecurityAIController::UpdateTargetFocus()
{
	const bool bShouldFocusTarget = TargetActor
		&& (bSecurityCalled || bReacquiredTargetDuringReturn)
		&& bSecurityChaseActive
		&& bHasSeenTarget
		&& !bReturning
		&& !bTargetCaptured
		&& !bIsStunned;

	if (bShouldFocusTarget)
	{
		SetFocus(TargetActor, EAIFocusPriority::Gameplay);
		return;
	}

	ClearFocus(EAIFocusPriority::Gameplay);
}

void AFTSecurityAIController::UpdateReturnTargetMemory()
{
	if (!bRememberingTarget)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || !TargetActor)
	{
		bRememberingTarget = false;
		bReacquiredTargetDuringReturn = false;
		TargetMemoryEndTime = 0.0f;
		TargetActor = nullptr;
		return;
	}

	if (World->GetTimeSeconds() > TargetMemoryEndTime)
	{
		bRememberingTarget = false;
		bReacquiredTargetDuringReturn = false;
		TargetMemoryEndTime = 0.0f;
		TargetActor = nullptr;
		return;
	}

	if (bHasSeenTarget)
	{
		StartPersonalRechase();
	}
}

void AFTSecurityAIController::StartPersonalRechase()
{
	if (!TargetActor)
	{
		return;
	}

	StopMovement();

	// 개인 재추격은 Event.Security.Called를 바로 방송하지 않고, 이 보안요원만 추격 상태로 복귀시킨다.
	bReturning = false;
	bReturnRequested = false;
	bRememberingTarget = false;
	TargetMemoryEndTime = 0.0f;
	bReacquiredTargetDuringReturn = true;
	bSecurityCalled = true;
	bSecurityChaseActive = true;
	SecurityChaseGauge = 100.0f;
	bHasObservedCrime = true;
	bCanRequestSecuritySupport = true;
	bReturnFailureLogged = false;
	bReturnCollisionIgnored = false;
	InvestigateLocation = TargetActor->GetActorLocation();

	if (SecurityCallComponent)
	{
		SecurityCallComponent->StartSecurityCall(TargetActor);
	}

	if (bLogSecurityEventDebug)
	{
		UE_LOG(
			LogFTSecurity,
			Log,
			TEXT("Security AI '%s' reacquired remembered target during return"),
			*GetName());
	}
}

void AFTSecurityAIController::UpdateChaseGaugeTargetSeenState()
{
	if (SecurityPursuitStateComponent)
	{
		SecurityPursuitStateComponent->UpdateTargetSeenState(this);
	}
}

void AFTSecurityAIController::OnChaseGaugeChanged(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	if (SecurityPursuitStateComponent)
	{
		SecurityPursuitStateComponent->HandleChaseGaugeChanged(this, Payload);
	}
}

void AFTSecurityAIController::UpdateSecurityCallGauge(float DeltaTime)
{
	const bool bShouldChargeSecurityCall = bCanRequestSecuritySupport
		&& (bSecurityCalled || bReacquiredTargetDuringReturn)
		&& bSecurityChaseActive
		&& bHasSeenTarget
		&& TargetActor
		&& !bReturning
		&& !bTargetCaptured
		&& !bIsStunned;

	if (SecurityCallComponent)
	{
		SecurityCallComponent->TickSecurityCall(DeltaTime, bShouldChargeSecurityCall, InvestigateLocation, bHasSeenTarget);
	}
}

void AFTSecurityAIController::OnChaseEnded(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	if (SecurityPursuitStateComponent)
	{
		SecurityPursuitStateComponent->HandleChaseEnded(this, Payload);
	}
}

void AFTSecurityAIController::OnSecurityDeployed(FGameplayTag Channel, const FFTSecurityResponsePayloadStruct& Payload)
{
	if (Payload.SecurityActor != GetPawn() || !Payload.TargetActor)
	{
		return;
	}

	bSpawnedFromSecurityRoom = true;
	bReturning = false;
	bTargetCaptured = false;
	bIsCaptor = false;
	bIsTargetCapturedByOtherSecurity = false;
	bReturnRequested = false;
	bInvestigateRequested = false;
	bStunRequested = false;
	bReturnFailureLogged = false;
	bReturnCollisionIgnored = false;
	bSecurityCalled = true;
	bSecurityChaseActive = true;
	bCanRequestSecuritySupport = false;
	if (SecurityCallComponent)
	{
		SecurityCallComponent->StopSecurityCall();
	}
	SecurityChaseGauge = 100.0f;
	SecurityRoomActor = Payload.SecurityRoomActor;
	HomeLocation = Payload.ReturnLocation;
	ReturnLocation = Payload.ReturnLocation;
	TargetActor = Payload.TargetActor;
	InvestigateLocation = Payload.ReportLocation.IsNearlyZero()
		? Payload.TargetActor->GetActorLocation()
		: Payload.ReportLocation;
	UpdateTargetState();

	if (bLogSecurityEventDebug)
	{
		UE_LOG(LogFTSecurity, Log, TEXT("Security AI '%s' deployed from room"), *GetName());
	}
}

void AFTSecurityAIController::OnSecurityTargetCaptured(
	FGameplayTag Channel,
	const FFTNPCReportPayloadStruct& Payload)
{
	if (SecurityCaptureStateComponent)
	{
		SecurityCaptureStateComponent->HandleTargetCaptured(this, Payload);
	}
}

void AFTSecurityAIController::OnSecurityTargetEscaped(
	FGameplayTag Channel,
	const FFTNPCReportPayloadStruct& Payload)
{
	if (SecurityCaptureStateComponent)
	{
		SecurityCaptureStateComponent->HandleTargetEscaped(this, Payload);
	}
}

void AFTSecurityAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (SecurityReturnComponent)
	{
		SecurityReturnComponent->HandleMoveCompleted(this, Result);
	}
}

bool AFTSecurityAIController::IsTargetCurrentlyVisible() const
{
	return IsActorVisibleBySight(TargetActor, SightConfig);
}

bool AFTSecurityAIController::IsActorDetectedByCloseRange(AActor* Actor) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !Actor || CloseDetectionRadius <= 0.0f)
	{
		return false;
	}

	// 전방 시야 밖이라도 아주 가까운 대상은 벽에 가려지지 않았을 때 기척으로 감지한다.
	const float DistanceSquared = FVector::DistSquared(
		ControlledPawn->GetActorLocation(),
		Actor->GetActorLocation()
	);
	return DistanceSquared <= FMath::Square(CloseDetectionRadius) && LineOfSightTo(Actor);
}

bool AFTSecurityAIController::IsTargetDetectedByCloseRange() const
{
	return IsActorDetectedByCloseRange(TargetActor);
}

bool AFTSecurityAIController::IsPlayerActor(const AActor* Actor) const
{
	const APawn* TargetPawn = Cast<APawn>(Actor);
	return TargetPawn && TargetPawn->IsPlayerControlled();
}

AActor* AFTSecurityAIController::ResolvePlayerActor(AActor* DamageCauser) const
{
	AActor* CurrentActor = DamageCauser;
	for (int32 OwnerDepth = 0; CurrentActor && OwnerDepth < 4; ++OwnerDepth)
	{
		if (IsPlayerActor(CurrentActor))
		{
			return CurrentActor;
		}

		if (APawn* InstigatorPawn = CurrentActor->GetInstigator(); IsPlayerActor(InstigatorPawn))
		{
			return InstigatorPawn;
		}

		CurrentActor = CurrentActor->GetOwner();
	}

	return nullptr;
}

bool AFTSecurityAIController::IsTargetStealing(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// 플레이어가 "훔치는" 채널링 중이면 ASC에 State.Stealing 태그가 부여된다(UFTChanneledInteractionComponent가 부여/회수).
	// 이전엔 IsChanneling()으로 판정해 채널 상호작용이면 무엇이든 도둑질로 오판정했으나, 이제 도둑질만 정확히 인식한다.
	AActor* MutableActor = const_cast<AActor*>(Actor);
	if (const IAbilitySystemInterface* AbilitySystemActor = Cast<IAbilitySystemInterface>(MutableActor))
	{
		if (const UAbilitySystemComponent* ASC = AbilitySystemActor->GetAbilitySystemComponent())
		{
			return ASC->HasMatchingGameplayTag(TAG_FT_State_Stealing);
		}
	}
	return false;
}

void AFTSecurityAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		if (AFTGameState* GameState = World->GetGameState<AFTGameState>())
		{
			GameState->SecurityCoordinationComponent->UnregisterSecurityController(this);
		}
	}

	if (SecurityPursuitStateComponent)
	{
		SecurityPursuitStateComponent->ClearReportedTargetSeen(this);
	}

	if (SecurityCalledListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(SecurityCalledListenerHandle);
	}
	if (ChaseGaugeChangedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(ChaseGaugeChangedListenerHandle);
	}
	if (ChaseEndedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(ChaseEndedListenerHandle);
	}
	if (SecurityDeployedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(SecurityDeployedListenerHandle);
	}
	if (SecurityTargetCapturedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(SecurityTargetCapturedListenerHandle);
	}
	if (SecurityTargetEscapedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(SecurityTargetEscapedListenerHandle);
	}
	if (ShelfDamagedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(ShelfDamagedListenerHandle);
	}
	if (CharacterAttackedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(CharacterAttackedListenerHandle);
	}

	Super::EndPlay(EndPlayReason);
}



void AFTSecurityAIController::DrawSightDebug() const
{
	DrawFlatSightDebug(SightConfig, FColor::Magenta, 1.5f);

	DrawFlatCircleDebug(CloseDetectionRadius, FColor::Yellow, 1.5f);

	if (bDrawAttackRangeDebug)
	{
		DrawFlatSectorDebug(AttackRange, SecurityPeripheralVisionAngle, FColor::Red, 2.5f);
	}
}
