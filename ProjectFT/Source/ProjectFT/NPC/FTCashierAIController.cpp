#include "FTCashierAIController.h"

#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Components/StateTreeAIComponent.h"
#include "ProjectFT/Components/FTInstantReportComponent.h"

AFTCashierAIController::AFTCashierAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	CashierStateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("CashierStateTreeAIComponent"));
	CashierPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("CashierPerceptionComponent"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	InstantReportComponent = CreateDefaultSubobject<UFTInstantReportComponent>(TEXT("InstantReportComponent"));
	ConfigureSight(CashierPerceptionComponent, SightConfig, 1200.0f, 60.0f, 2.0f);
}

void AFTCashierAIController::BeginPlay()
{
	Super::BeginPlay();

	RefreshSightConfig(CashierPerceptionComponent, SightConfig);

	if (CashierPerceptionComponent)
	{
		CashierPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::OnTargetPerceptionUpdated);
	}

	if (InstantReportComponent)
	{
		InstantReportComponent->Initialize(this, SightConfig);
		InstantReportComponent->bReportOnlyOnce = bReportOnlyOnce;
		InstantReportComponent->bLogInstantReportDebug = bLogCashierDebug;
		InstantReportComponent->StartListening();
		SyncInstantReportStateFromComponent();
	}
}

void AFTCashierAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InstantReportComponent)
	{
		InstantReportComponent->StopListening();
	}

	Super::EndPlay(EndPlayReason);
}

void AFTCashierAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (InstantReportComponent)
	{
		InstantReportComponent->bReportOnlyOnce = bReportOnlyOnce;
		InstantReportComponent->bLogInstantReportDebug = bLogCashierDebug;
		InstantReportComponent->TickInstantReport();
		SyncInstantReportStateFromComponent();
	}
	DrawFlatSightDebug(SightConfig, FColor::Yellow, 1.5f);
}

void AFTCashierAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!IsPlayerActor(Actor))
	{
		return;
	}

	if (InstantReportComponent)
	{
		InstantReportComponent->HandleTargetPerceptionUpdated(Actor);
		SyncInstantReportStateFromComponent();
	}
}

bool AFTCashierAIController::BroadcastRequestedReport()
{
	if (!InstantReportComponent)
	{
		return false;
	}

	InstantReportComponent->bReportOnlyOnce = bReportOnlyOnce;
	InstantReportComponent->bLogInstantReportDebug = bLogCashierDebug;
	const bool bBroadcasted = InstantReportComponent->BroadcastRequestedReport();
	SyncInstantReportStateFromComponent();

	return bBroadcasted;
}

bool AFTCashierAIController::IsPlayerActor(const AActor* Actor) const
{
	const APawn* TargetPawn = Cast<APawn>(Actor);
	return TargetPawn && TargetPawn->IsPlayerControlled();
}

void AFTCashierAIController::SyncInstantReportStateFromComponent()
{
	if (!InstantReportComponent)
	{
		return;
	}

	TargetActor = InstantReportComponent->TargetActor;
	bHasSeenTarget = InstantReportComponent->bHasSeenTarget;
	bHasReported = InstantReportComponent->bHasReported;
	bReportRequested = InstantReportComponent->bReportRequested;
}
