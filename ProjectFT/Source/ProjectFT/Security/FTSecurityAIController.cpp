
#include "FTSecurityAIController.h"

#include "DrawDebugHelpers.h"
#include "Components/StateTreeAIComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionTypes.h"

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
	
	SecurityPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("SecurityPerceptionComponent"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 80.0f;
	SightConfig->SetMaxAge(3.0f);
	
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	
	SecurityPerceptionComponent->ConfigureSense(*SightConfig);
	SecurityPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*SecurityPerceptionComponent);
}

void AFTSecurityAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTargetState();
	DrawSightDebug();
}

void AFTSecurityAIController::BeginPlay()
{
	Super::BeginPlay();

	// Event.Security.Called 메시지가 발행될 때마다 OnSecurityCalled()가 호출됨.
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	SecurityCalledListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_SecurityCalled, 
		this, 
		&ThisClass::OnSecurityCalled
	);

	APawn* ControllPawn = GetPawn();
	if (!ControllPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: ControlledPawn is null"));
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
	if (!Actor || Actor != TargetActor)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
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
	TargetActor = NewTargetActor;
}

void AFTSecurityAIController::StartChase()
{
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: TargetActor is null"));
		return;
	}

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(TargetActor, 150.0f);
	UE_LOG(LogTemp, Log, TEXT("Security AI: MoveToActor result %d"), static_cast<int32>(MoveResult));
}

void AFTSecurityAIController::OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
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

AActor* AFTSecurityAIController::GetTargetActor() const
{
	return TargetActor;
}

void AFTSecurityAIController::UpdateTargetState()
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !TargetActor)
	{
		TargetDistance = 0.0f;
		bHasSeenTarget = false;
		bIsTargetInAttackRange = false;
		return;
	}

	TargetDistance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	bHasSeenTarget = IsTargetCurrentlyVisible();
	bIsTargetInAttackRange = bHasSeenTarget && TargetDistance <= AttackRange;
	
	// 현재 보이는 상태라면 마지막 목격 위치 갱신
	if (bHasSeenTarget)
	{
		InvestigateLocation = TargetActor->GetActorLocation();
	}
}

bool AFTSecurityAIController::IsTargetCurrentlyVisible() const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !TargetActor || !SightConfig)
	{
		return false;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
	if (ToTarget.SizeSquared() > FMath::Square(SightConfig->LoseSightRadius))
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

void AFTSecurityAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SecurityCalledListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(SecurityCalledListenerHandle);
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

	const FVector EyeLocation = ControlledPawn->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	const FVector Forward = ControlledPawn->GetActorForwardVector();
	const float ConeHalfAngleRadians = FMath::DegreesToRadians(SightConfig->PeripheralVisionAngleDegrees);

	DrawDebugSphere(
		GetWorld(),
		ControlledPawn->GetActorLocation(),
		SightConfig->SightRadius,
		32,
		FColor::Green,
		false,
		0.05f,
		0,
		1.5f
	);

	DrawDebugSphere(
		GetWorld(),
		ControlledPawn->GetActorLocation(),
		SightConfig->LoseSightRadius,
		32,
		FColor::Yellow,
		false,
		0.05f,
		0,
		1.5f
	);

	DrawDebugSphere(
		GetWorld(),
		ControlledPawn->GetActorLocation(),
		AttackRange,
		24,
		FColor::Red,
		false,
		0.05f,
		0,
		2.5f
	);

	DrawDebugCone(
		GetWorld(),
		EyeLocation,
		Forward,
		SightConfig->SightRadius,
		ConeHalfAngleRadians,
		ConeHalfAngleRadians,
		24,
		FColor::Cyan,
		false,
		0.05f,
		0,
		2.0f
	);
}
