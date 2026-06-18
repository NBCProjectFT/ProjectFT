
#include "FTSecurityAIController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionTypes.h"


AFTSecurityAIController::AFTSecurityAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	
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

void AFTSecurityAIController::BeginPlay()
{
	Super::BeginPlay();
	APawn* ControllPawn = GetPawn();
	if (!ControllPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: ControlledPawn is null"));
		return;
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
	
	// Event.Security.Called 메시지가 발행될 때마다 OnSecurityCalled()가 호출됨.
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	SecurityCalledListenerHandle = MessageSubsystem.RegisterListener(TAG_FT_Event_SecurityCalled, this, &ThisClass::OnSecurityCalled);
}

void AFTSecurityAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || Actor != TargetActor)
	{
		return;
	}
	
	if (Stimulus.WasSuccessfullySensed())
	{
		MoveToActor(TargetActor);
		UE_LOG(LogTemp, Log, TEXT("Security AI: Target sensed, chasing"));
	}
	else
	{
		const FVector DetectedLocation = Stimulus.StimulusLocation;
		MoveToLocation(DetectedLocation);
		UE_LOG(LogTemp, Log, TEXT("Security AI: Target lost, moving to last known location"));
	}
	
	// 공격 로직을 짜기 전, 확인용 임시 코드. 실제 HP 감소는 나중에 연결
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: ControlledPawn is null"));
		return;
	}

	const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance <= 150.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("Security AI: Attack range reached"));
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

	MoveToActor(TargetActor);
}

void AFTSecurityAIController::OnSecurityCalled(FGameplayTag Channel, const FFTNPCReportPayloadStruct& Payload)
{
	if (!Payload.TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Security AI: Security called but TargetActor is null"));
		return;
	}
	SetTargetActor(Payload.TargetActor);
	StartChase();
	
	UE_LOG(LogTemp, Log, TEXT("Security AI: Security called, chasing %s"), *Payload.TargetActor->GetName());
}

void AFTSecurityAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SecurityCalledListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(SecurityCalledListenerHandle);
	}

	Super::EndPlay(EndPlayReason);
}
