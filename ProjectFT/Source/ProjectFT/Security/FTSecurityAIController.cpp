
#include "FTSecurityAIController.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/StateTreeAIComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
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

// 그저 테스트용
// TODO: 테스트 완료 후 제거. NPC가 FTReportGaugeComponent애서 Call 하는 로직으로 변경 예정.
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
	
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = SightConfig->SightRadius;
	SightConfig->PeripheralVisionAngleDegrees = 80.0f;
	SightConfig->SetMaxAge(3.0f);
	
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	
	SecurityPerceptionComponent->ConfigureSense(*SightConfig);
	SecurityPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*SecurityPerceptionComponent);
}

void AFTSecurityAIController::PreInitializeComponents()
{
	SecurityStateTreeAIComponent->SetStartLogicAutomatically(false);
	Super::PreInitializeComponents();
}

void AFTSecurityAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn && !bSpawnedFromSecurityRoom)
	{
		HomeLocation = InPawn->GetActorLocation();
		ReturnLocation = HomeLocation;
	}

	if (SecurityStateTreeAIComponent)
	{
		SecurityStateTreeAIComponent->StartLogic();
	}
}

void AFTSecurityAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAbilityState();
	UpdateTargetState();
	UpdateTargetFocus();
	UpdateReturnCollision();
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

	if (SightConfig && SecurityPerceptionComponent)
	{
		SightConfig->LoseSightRadius = SightConfig->SightRadius;
		SecurityPerceptionComponent->RequestStimuliListenerUpdate();
	}
	

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

	if (APawn* ControlledPawn = GetPawn())
	{
		HomeLocation = ControlledPawn->GetActorLocation();
		ReturnLocation = HomeLocation;
	}
	
	/*
	const FVector StartLocation = ControllPawn->GetActorLocation();
	const FVector TargetLocation = StartLocation + FVector(500.0f, 0.0f, 0.0f);
	
	MoveToLocation(TargetLocation);
	UE_LOG(LogTemp, Warning, TEXT("Security AI: MoveToLocation Test Started"));
	*/
	
	/*
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this,0);
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: PlayerPawn is null"));
		return;
	}
	MoveToActor(PlayerPawn);
	*/
	
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
			InvestigateLocation = Actor->GetActorLocation();
			UE_LOG(LogTemp, Log, TEXT("Security AI: Direct theft witnessed, chasing %s"), *GetNameSafe(Actor));
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
			UE_LOG(LogTemp, Log, TEXT("Security AI: Attack range reached"));
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
		UE_LOG(LogTemp, Log, TEXT("Security AI: Target lost, moving to last known location"));
	}
}

void AFTSecurityAIController::SetTargetActor(AActor* NewTargetActor)
{
	if (TargetActor != NewTargetActor)
	{
		LastTargetVisibleTime = -BIG_NUMBER;
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

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(TargetActor, 150.0f);
	UE_LOG(LogTemp, Log, TEXT("Security AI: MoveToActor result %d"), static_cast<int32>(MoveResult));
}

void AFTSecurityAIController::OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	if (bTargetCaptured)
	{
		return;
	}

	if (!Payload.TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: TargetActor is null"));
		return;
	}

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !SightConfig)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: ControlledPawn or SightConfig is null"));
		return;
	}

	if (bReturning)
	{
		StopMovement();
	}
	bReturning = false;
	bReturnRequested = false;
	bInvestigateRequested = false;
	bStunRequested = false;
	bReturnFailureLogged = false;
	bReturnCollisionIgnored = false;
	if (AFTSecurityCharacter* SecurityCharacter = Cast<AFTSecurityCharacter>(GetPawn()))
	{
		SecurityCharacter->RestorePawnCollision();
	}
	bSecurityCalled = true;
	SetTargetActor(Payload.TargetActor);
	InvestigateLocation = Payload.ReportLocation.IsNearlyZero() ? Payload.TargetActor->GetActorLocation() : Payload.ReportLocation;
	UpdateTargetState();

	const float DistanceToTarget = FVector::Dist(ControlledPawn->GetActorLocation(), Payload.TargetActor->GetActorLocation());

	if (DistanceToTarget <= SightConfig->LoseSightRadius)
	{
		// StartChase();
		// UE_LOG(LogTemp, Log, TEXT("Security AI: Target in range, chasing %s"), *Payload.TargetActor->GetName());
		// return;
	}

	// const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(InvestigateLocation, 150.0f);
	// UE_LOG(LogTemp, Log, TEXT("Security AI: Investigating location %s, result %d"), *InvestigateLocation.ToString(), static_cast<int32>(MoveResult));
}

void AFTSecurityAIController::OnShelfDamaged(
	FGameplayTag Channel,
	const FFTMessagePayloadStruct& Payload)
{
	if (bTargetCaptured || bIsStunned)
	{
		return;
	}

	AActor* SuspectActor = ResolvePlayerActor(Payload.InstigatorActor);
	AActor* DamagedShelf = Payload.TargetActor;
	if (!SuspectActor)
	{
		// TODO: 공격 측에서 DamageCauser를 정상 전달하면 싱글플레이용 fallback을 제거한다.
		SuspectActor = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	if (!SuspectActor || !DamagedShelf)
	{
		UE_LOG(
			LogFTSecurity,
			Warning,
			TEXT("Security AI ignored shelf damage: Instigator=%s Player=%s Shelf=%s"),
			*GetNameSafe(Payload.InstigatorActor),
			*GetNameSafe(SuspectActor),
			*GetNameSafe(DamagedShelf));
		return;
	}

	SetTargetActor(SuspectActor);
	const bool bCanSeePlayer = IsTargetCurrentlyVisible();
	const bool bCanSeeDamagedShelf = LineOfSightTo(DamagedShelf);
	if (!bCanSeePlayer || !bCanSeeDamagedShelf)
	{
		return;
	}

	bReturning = false;
	bReturnRequested = false;
	bSecurityCalled = true;
	InvestigateLocation = SuspectActor->GetActorLocation();
	UpdateTargetState();

	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Security AI '%s' witnessed shelf damage, chasing %s"),
		*GetName(),
		*GetNameSafe(SuspectActor));
}

AActor* AFTSecurityAIController::GetTargetActor() const
{
	return TargetActor;
}

void AFTSecurityAIController::UpdateTargetState()
{
	if (bTargetCaptured)
	{
		TargetDistance = 0.0f;
		bHasSeenTarget = false;
		bIsTargetInAttackRange = false;
		UpdateChaseGaugeTargetSeenState();
		return;
	}

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !TargetActor)
	{
		TargetDistance = 0.0f;
		bHasSeenTarget = false;
		bIsTargetInAttackRange = false;
		LastTargetVisibleTime = -BIG_NUMBER;
		UpdateChaseGaugeTargetSeenState();
		return;
	}

	TargetDistance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const bool bTargetCurrentlyVisible = IsTargetCurrentlyVisible();
	if (bTargetCurrentlyVisible)
	{
		LastTargetVisibleTime = GetWorld()->GetTimeSeconds();
	}

	const float TimeSinceTargetVisible = GetWorld()->GetTimeSeconds() - LastTargetVisibleTime;
	bHasSeenTarget = bTargetCurrentlyVisible || TimeSinceTargetVisible <= TargetSightLostGracePeriod;
	bIsTargetInAttackRange = bHasSeenTarget && TargetDistance <= AttackRange;

	if (!bSecurityCalled && bHasSeenTarget && IsTargetStealing(TargetActor))
	{
		bReturning = false;
		bSecurityCalled = true;
		InvestigateLocation = TargetActor->GetActorLocation();
		UE_LOG(LogTemp, Log, TEXT("Security AI: Direct theft witnessed, chasing %s"), *GetNameSafe(TargetActor));
	}
	
	// 현재 보이는 상태라면 마지막 목격 위치 갱신
	if (bHasSeenTarget)
	{
		InvestigateLocation = TargetActor->GetActorLocation();
	}

	UpdateChaseGaugeTargetSeenState();
}

void AFTSecurityAIController::UpdateTargetFocus()
{
	const bool bShouldFocusTarget = TargetActor
		&& bSecurityCalled
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

void AFTSecurityAIController::UpdateChaseGaugeTargetSeenState()
{
	const bool bShouldReportTargetSeen = bSecurityCalled && bHasSeenTarget;
	if (bReportedTargetSeenToChaseGauge == bShouldReportTargetSeen)
	{
		return;
	}

	FFTSecurityChaseGaugePayloadStruct Payload;
	Payload.SecurityActor = GetPawn();
	Payload.TargetActor = TargetActor;
	Payload.LastKnownLocation = InvestigateLocation;
	Payload.bHasSeenTarget = bShouldReportTargetSeen;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		bShouldReportTargetSeen ? TAG_FT_Event_SecurityTargetSeen : TAG_FT_Event_SecurityTargetLost,
		Payload
	);
	bReportedTargetSeenToChaseGauge = bShouldReportTargetSeen;
}

void AFTSecurityAIController::OnChaseGaugeChanged(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	SecurityChaseGauge = Payload.ChaseGauge;
	bSecurityChaseActive = SecurityChaseGauge > 0.0f;
}

void AFTSecurityAIController::OnChaseEnded(FGameplayTag Channel, const FFTSecurityChaseGaugePayloadStruct& Payload)
{
	if (bTargetCaptured)
	{
		return;
	}

	StopMovement();
	bReturning = true;
	bReturnRequested = true;
	bReturnFailureLogged = false;
	bReturnCollisionIgnored = false;
	SecurityChaseGauge = 0.0f;
	bSecurityChaseActive = false;
	bSecurityCalled = false;
	ClearFocus(EAIFocusPriority::Gameplay);
	TargetActor = nullptr;
	LastTargetVisibleTime = -BIG_NUMBER;
	TargetDistance = 0.0f;
	bHasSeenTarget = false;
	bIsTargetInAttackRange = false;
	bReportedTargetSeenToChaseGauge = false;

	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Security AI '%s' requested return to %s"),
		*GetName(),
		*ReturnLocation.ToString());
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
	SecurityChaseGauge = 100.0f;
	SecurityRoomActor = Payload.SecurityRoomActor;
	HomeLocation = Payload.ReturnLocation;
	ReturnLocation = Payload.ReturnLocation;
	TargetActor = Payload.TargetActor;
	InvestigateLocation = Payload.ReportLocation.IsNearlyZero()
		? Payload.TargetActor->GetActorLocation()
		: Payload.ReportLocation;
	UpdateTargetState();

	UE_LOG(LogFTSecurity, Log, TEXT("Security AI '%s' deployed from room"), *GetName());
}

void AFTSecurityAIController::OnSecurityTargetCaptured(
	FGameplayTag Channel,
	const FFTNPCReportPayloadStruct& Payload)
{
	if (!Payload.ReporterActor || !Payload.TargetActor || !GetPawn())
	{
		return;
	}

	StopMovement();
	TargetActor = Payload.TargetActor;
	InvestigateLocation = Payload.ReportLocation;
	bTargetCaptured = true;
	bIsCaptor = Payload.ReporterActor == GetPawn();
	bIsTargetCapturedByOtherSecurity = !bIsCaptor;
	bReturnRequested = !bIsCaptor;
	bReturning = !bIsCaptor;
	bInvestigateRequested = false;
	bStunRequested = false;
	bSecurityCalled = false;
	bHasSeenTarget = false;
	bIsTargetInAttackRange = false;
	UpdateChaseGaugeTargetSeenState();

	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Security AI '%s' received target captured: Captor=%s IsCaptor=%s"),
		*GetName(),
		*GetNameSafe(Payload.ReporterActor),
		bIsCaptor ? TEXT("true") : TEXT("false")
	);
}

void AFTSecurityAIController::OnSecurityTargetEscaped(
	FGameplayTag Channel,
	const FFTNPCReportPayloadStruct& Payload)
{
	if (!Payload.ReporterActor || !Payload.TargetActor)
	{
		return;
	}

	const bool bWasEscapedFromThisSecurity = Payload.ReporterActor == GetPawn();
	StopMovement();
	TargetActor = Payload.TargetActor;
	InvestigateLocation = Payload.ReportLocation.IsNearlyZero()
		? Payload.TargetActor->GetActorLocation()
		: Payload.ReportLocation;
	bTargetCaptured = false;
	bIsCaptor = false;
	bIsTargetCapturedByOtherSecurity = false;
	bReturnRequested = false;
	bReturning = false;
	bInvestigateRequested = true;
	bStunRequested = bWasEscapedFromThisSecurity;
	bSecurityCalled = true;
	bReturnFailureLogged = false;
	bReturnCollisionIgnored = false;

	if (AFTSecurityCharacter* SecurityCharacter = Cast<AFTSecurityCharacter>(GetPawn()))
	{
		SecurityCharacter->RestorePawnCollision();
	}

	UpdateTargetState();
	UE_LOG(
		LogFTSecurity,
		Log,
		TEXT("Security AI '%s' received target escaped: StunRequested=%s Location=%s"),
		*GetName(),
		bStunRequested ? TEXT("true") : TEXT("false"),
		*InvestigateLocation.ToString()
	);
}

void AFTSecurityAIController::UpdateReturnCollision()
{
	if (!bReturning || !bSpawnedFromSecurityRoom || bReturnCollisionIgnored)
	{
		return;
	}

	AFTSecurityCharacter* SecurityCharacter = Cast<AFTSecurityCharacter>(GetPawn());
	if (!SecurityCharacter)
	{
		return;
	}

	if (FVector::DistSquared(SecurityCharacter->GetActorLocation(), ReturnLocation)
		> FMath::Square(ReturnCollisionIgnoreDistance))
	{
		return;
	}

	bReturnCollisionIgnored = true;
	SecurityCharacter->IgnorePawnCollisionForDuration(0.0f);
}

void AFTSecurityAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (!bReturning)
	{
		return;
	}

	// 추격 또는 EQS 이동을 중단한 결과는 복귀 이동 실패가 아니다.
	if (Result.Code == EPathFollowingResult::Aborted)
	{
		return;
	}

	if (!Result.IsSuccess())
	{
		if (!bReturnFailureLogged)
		{
			bReturnFailureLogged = true;
			UE_LOG(LogFTSecurity, Warning, TEXT("Security AI '%s' failed to return"), *GetName());
		}
		return;
	}

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const float DistanceToReturnLocation = FVector::Dist2D(
		ControlledPawn->GetActorLocation(),
		ReturnLocation);
	if (DistanceToReturnLocation > ReturnCompletionDistance)
	{
		if (!bReturnFailureLogged)
		{
			bReturnFailureLogged = true;
			UE_LOG(
				LogFTSecurity,
				Warning,
				TEXT("Security AI '%s' completed an unrelated move while returning: Distance=%.1f"),
				*GetName(),
				DistanceToReturnLocation);
		}
		return;
	}

	bReturnFailureLogged = false;
	CompleteReturn();
}

void AFTSecurityAIController::CompleteReturn()
{
	bReturning = false;
	bReturnRequested = false;
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (!bSpawnedFromSecurityRoom)
	{
		UE_LOG(LogFTSecurity, Log, TEXT("Security AI '%s' returned home"), *GetName());
		return;
	}

	FFTSecurityResponsePayloadStruct Payload;
	Payload.SecurityActor = ControlledPawn;
	Payload.SecurityRoomActor = SecurityRoomActor;
	Payload.ReturnLocation = ReturnLocation;

	UE_LOG(LogFTSecurity, Log, TEXT("Security AI '%s' returned to security room"), *GetName());
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Event_SecurityReturnedToRoom, Payload);
}

bool AFTSecurityAIController::IsTargetCurrentlyVisible() const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !TargetActor || !SightConfig)
	{
		return false;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
	if (ToTarget.SizeSquared() > FMath::Square(SightConfig->SightRadius))
	{
		return false;
	}

	const FVector Forward = ControlledPawn->GetActorForwardVector();
	const FVector DirectionToTarget = ToTarget.GetSafeNormal();
	const float Dot = FVector::DotProduct(Forward, DirectionToTarget);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
	if (AngleDegrees > SightConfig->PeripheralVisionAngleDegrees)
	{
		return false;
	}

	return LineOfSightTo(TargetActor);
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

	if (bReportedTargetSeenToChaseGauge)
	{
		bHasSeenTarget = false;
		UpdateChaseGaugeTargetSeenState();
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

	Super::EndPlay(EndPlayReason);
}



// 눈으로 보는 확인용.
void AFTSecurityAIController::DrawSightDebug() const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !SightConfig)
	{
		return;
	}

	const FVector Origin = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 8.0f);
	const FVector Forward = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	constexpr int32 SegmentCount = 16;
	constexpr float LifeTime = 0.05f;

	auto DrawFlatSector = [this, Origin, Forward, SegmentCount, LifeTime](float Radius, float HalfAngleDegrees, FColor Color, float Thickness)
	{
		FVector PreviousPoint = Origin;
		for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float AngleDegrees = FMath::Lerp(-HalfAngleDegrees, HalfAngleDegrees, Alpha);
			const FVector Direction = Forward.RotateAngleAxis(AngleDegrees, FVector::UpVector);
			const FVector CurrentPoint = Origin + Direction * Radius;

			if (SegmentIndex == 0)
			{
				DrawDebugLine(GetWorld(), Origin, CurrentPoint, Color, false, LifeTime, 0, Thickness);
			}
			else
			{
				DrawDebugLine(GetWorld(), PreviousPoint, CurrentPoint, Color, false, LifeTime, 0, Thickness);
			}

			if (SegmentIndex == SegmentCount)
			{
				DrawDebugLine(GetWorld(), Origin, CurrentPoint, Color, false, LifeTime, 0, Thickness);
			}

			PreviousPoint = CurrentPoint;
		}
	};

	DrawFlatSector(SightConfig->SightRadius, SightConfig->PeripheralVisionAngleDegrees, FColor::Magenta, 1.5f);
	DrawFlatSector(AttackRange, SightConfig->PeripheralVisionAngleDegrees, FColor::Red, 2.5f);
}
