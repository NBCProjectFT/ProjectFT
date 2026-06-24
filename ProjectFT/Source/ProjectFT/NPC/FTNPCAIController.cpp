
#include "FTNPCAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"

namespace
{
	FFTNPCReportPayloadStruct MakeNPCReportPayload(const AFTNPCAIController* Controller, AActor* TargetActor, float ReportAmount, float ReportProgress)
	{
		FFTNPCReportPayloadStruct Payload;
		Payload.ReporterActor = Controller ? Controller->GetPawn() : nullptr;
		Payload.TargetActor = TargetActor;
		Payload.ReportLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
		Payload.ReportAmount = ReportAmount;
		Payload.ReportProgress = ReportProgress;

		return Payload;
	}

	void BroadcastNPCReportMessage(const AFTNPCAIController* Controller, FGameplayTag Channel, AActor* TargetActor, float ReportAmount, float ReportProgress)
	{
		if (!Controller)
		{
			return;
		}

		const FFTNPCReportPayloadStruct Payload = MakeNPCReportPayload(Controller, TargetActor, ReportAmount, ReportProgress);
		UGameplayMessageSubsystem::Get(Controller).BroadcastMessage(Channel, Payload);
	}
}


AFTNPCAIController::AFTNPCAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	//NPC용 StateTree Component 추가
	NPCStateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("NPCStateTreeAIComponent"));
	
	NPCPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("NPCPerceptionComponent"));
	
	//NPC가 플레이어의 도둑질을 인식할 시야각 추가
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 40.0f;
	SightConfig->SetMaxAge(2.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	NPCPerceptionComponent->ConfigureSense(*SightConfig);
	NPCPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*NPCPerceptionComponent);
}	


void AFTNPCAIController::BeginPlay()
{
	Super::BeginPlay();

	if (NPCPerceptionComponent)
	{
		NPCPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AFTNPCAIController::OnTargetPerceptionUpdated);
	}
}


void AFTNPCAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTargetState();
	DrawSightDebug();
}

void AFTNPCAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!IsPlayerActor(Actor))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		TargetActor = Actor;
		UpdateTargetState();
		BroadcastNPCReportMessage(this, TAG_FT_Event_NPCDetectedPlayer, TargetActor, 0.0f, 0.0f);
		UE_LOG(LogFTNPC, Log, TEXT("NPC AI: Target sensed %s"), *GetNameSafe(Actor));
		return;
	}

	if (Actor == TargetActor)
	{
		UpdateTargetState();
		UE_LOG(LogFTNPC, Log, TEXT("NPC AI: Target lost %s"), *GetNameSafe(Actor));
	}
}

bool AFTNPCAIController::PickRandomShoppingTarget()
{
	TArray<AActor*> ShoppingPoints;
	UGameplayStatics::GetAllActorsWithTag(this, ShoppingPointTag, ShoppingPoints);

	if (ShoppingPoints.IsEmpty())
	{
		ShoppingTargetLocation = FVector::ZeroVector;
		bHasShoppingTarget = false;
		UE_LOG(LogFTNPC, Warning, TEXT("NPC AI: No shopping point found with tag %s"), *ShoppingPointTag.ToString());
		return false;
	}

	const int32 TargetIndex = FMath::RandRange(0, ShoppingPoints.Num() - 1);
	const AActor* SelectedShoppingPoint = ShoppingPoints[TargetIndex];
	if (!SelectedShoppingPoint)
	{
		ShoppingTargetLocation = FVector::ZeroVector;
		bHasShoppingTarget = false;
		UE_LOG(LogFTNPC, Warning, TEXT("NPC AI: Selected shopping point is invalid"));
		return false;
	}

	ShoppingTargetLocation = SelectedShoppingPoint->GetActorLocation();
	bHasShoppingTarget = true;

	UE_LOG(LogFTNPC, Log, TEXT("NPC AI: Picked shopping target %s at %s"), *SelectedShoppingPoint->GetName(), *ShoppingTargetLocation.ToString());
	return true;
}

bool AFTNPCAIController::PickRandomWanderTarget()
{
	return PickRandomShoppingTarget();
}

void AFTNPCAIController::EnterSuspicious()
{
	UpdateTargetState();
	UE_LOG(LogFTNPC, Log, TEXT("[NPC] Enter Suspicious"));
}

void AFTNPCAIController::EnterReporting()
{
	ReportElapsedTime = 0.0f;
	CurrentReportProgress = 0.0f;
	bReportCompleted = false;
	bReportCancelled = false;
	LastLoggedReportPercent = -1;
	LastLoggedReportDecayPercent = 101;

	UpdateTargetState();
	UE_LOG(LogFTNPC, Log, TEXT("[NPC] Enter Reporting"));
	BroadcastNPCReportMessage(this, TAG_FT_Event_NPCReportStarted, TargetActor, 0.0f, 0.0f);

	if (ShouldCancelReport())
	{
		CancelReport();
	}
}

bool AFTNPCAIController::TickReporting(float DeltaTime)
{
	if (bReportCompleted)
	{
		return true;
	}

	if (bReportCancelled)
	{
		return false;
	}

	UpdateTargetState();
	if (ShouldCancelReport())
	{
		CancelReport();
		return false;
	}
	
	if (!bIsTargetActivelyStealing)
	{
		if (CurrentReportProgress > 0.0f)
		{
			ReportElapsedTime = FMath::Max(
				ReportElapsedTime - DeltaTime * (ReportDuration / FMath::Max(ReportDecayDuration, KINDA_SMALL_NUMBER)),
				0.0f
			);
			CurrentReportProgress = FMath::Clamp(ReportElapsedTime / FMath::Max(ReportDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
			BroadcastNPCReportMessage(this, TAG_FT_Event_NPCReportProgress, TargetActor, ReportAmount, CurrentReportProgress);

			const int32 ReportPercent = FMath::FloorToInt(CurrentReportProgress * 100.0f);
			if (ReportPercent / 10 < LastLoggedReportDecayPercent / 10)
			{
				LastLoggedReportDecayPercent = ReportPercent;
				UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Decay %d%%"), ReportPercent);
			}
		}

		if (CurrentReportProgress <= 0.0f)
		{
			CancelReport();
		}

		return false;
	}

	ReportElapsedTime += DeltaTime;
	CurrentReportProgress = FMath::Clamp(ReportElapsedTime / FMath::Max(ReportDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	BroadcastNPCReportMessage(this, TAG_FT_Event_NPCReportProgress, TargetActor, ReportAmount, CurrentReportProgress);
	LastLoggedReportDecayPercent = 101;

	const int32 ReportPercent = FMath::FloorToInt(CurrentReportProgress * 100.0f);
	if (ReportPercent / 10 > LastLoggedReportPercent / 10)
	{
		LastLoggedReportPercent = ReportPercent;
		UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Progress %d%%"), ReportPercent);
	}

	if (CurrentReportProgress >= 1.0f)
	{
		CompleteReport();
		return true;
	}

	return false;
}

void AFTNPCAIController::CancelReport()
{
	if (bReportCompleted || bReportCancelled)
	{
		return;
	}

	bReportCancelled = true;
	CurrentReportProgress = 0.0f;
	ReportElapsedTime = 0.0f;
	LastLoggedReportDecayPercent = 0;
	BroadcastNPCReportMessage(this, TAG_FT_Event_NPCReportProgress, TargetActor, ReportAmount, 0.0f);
	UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Cancelled"));
}

void AFTNPCAIController::UpdateTargetState()
{
	if (!TargetActor)
	{
		TargetDistance = 0.0f;
		bHasSeenTarget = false;
		bIsTargetStealing = false;
		bIsTargetActivelyStealing = false;
		bCanStartReportFlow = false;
		return;
	}

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		TargetDistance = 0.0f;
		bHasSeenTarget = false;
		bIsTargetStealing = false;
		bIsTargetActivelyStealing = false;
		bCanStartReportFlow = false;
		return;
	}

	TargetDistance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	bHasSeenTarget = IsTargetCurrentlyVisible();

	bIsTargetActivelyStealing = IsTargetStealing(TargetActor);
	if (bHasSeenTarget && bIsTargetActivelyStealing)
	{
		LastObservedStealingTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastObservedStealingTime;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bRecentlyObservedStealing = CurrentTime - LastObservedStealingTime <= ObservedStealingMemorySeconds;
	bIsTargetStealing = bIsTargetActivelyStealing || bRecentlyObservedStealing;
	bCanStartReportFlow = TargetActor && bHasSeenTarget && bIsTargetActivelyStealing;

	LogReportConditionDebug(bIsTargetActivelyStealing);
}

bool AFTNPCAIController::CanStartReportFlow() const
{
	return bCanStartReportFlow;
}

bool AFTNPCAIController::IsPlayerActor(const AActor* Actor) const
{
	const APawn* TargetPawn = Cast<APawn>(Actor);
	return TargetPawn && TargetPawn->IsPlayerControlled();
}

bool AFTNPCAIController::IsTargetCurrentlyVisible() const
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

bool AFTNPCAIController::IsTargetStealing(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// TODO: 플레이어 쪽에서 도둑질 중 상태를 GameplayTag로 제공해주시길 바랍니다.
	// TODO: 플레이어 담당 코드에 명시적인 도둑질 상태가 생기면 이 임시 판정을 교체.
	// 현재는 플레이어 코드를 수정하지 않기 위해 임시로 채널링 상호작용 중인지로만 도둑질을 판정한다.
	// 그러나 이 방식은 채널링 상호작용이기만 하면 전부 도둑질로 판단해버리니
	// 플레이어 쪽에서 도둑질 중 상태를 GameplayTag로 제공해주시길 바랍니다.
	const UFTInteractionComponent* InteractionComponent = Actor->FindComponentByClass<UFTInteractionComponent>();
	return InteractionComponent && InteractionComponent->IsChanneling();
}

bool AFTNPCAIController::ShouldCancelReport() const
{
	return !TargetActor ||
		!bHasSeenTarget ||
		// !bIsTargetStealing
		TargetDistance > ReportCancelDistance;
}

void AFTNPCAIController::CompleteReport()
{
	if (bReportCompleted || bReportCancelled)
	{
		return;
	}

	bReportCompleted = true;
	CurrentReportProgress = 1.0f;

	BroadcastNPCReportMessage(this, TAG_FT_Event_NPCReportCompleted, TargetActor, ReportAmount, 1.0f);
	UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Completed"));
}

void AFTNPCAIController::DrawSightDebug() const
{
	if (!bDrawSightDebug)
	{
		return;
	}

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
		ReportCancelDistance,
		32,
		FColor::Orange,
		false,
		0.05f,
		0,
		2.0f
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

void AFTNPCAIController::LogReportConditionDebug(bool bTargetCurrentlyStealing)
{
	if (bLastLoggedHasSeenTarget == bHasSeenTarget &&
		bLastLoggedIsTargetStealing == bIsTargetStealing &&
		bLastLoggedCanStartReportFlow == bCanStartReportFlow)
	{
		return;
	}

	bLastLoggedHasSeenTarget = bHasSeenTarget;
	bLastLoggedIsTargetStealing = bIsTargetStealing;
	bLastLoggedCanStartReportFlow = bCanStartReportFlow;

	UE_LOG(
		LogFTNPC,
		Log,
		TEXT("NPC AI: Report conditions Target=%s Seen=%s Stealing=%s ChannelingNow=%s Distance=%.1f CanStart=%s"),
		*GetNameSafe(TargetActor),
		bHasSeenTarget ? TEXT("true") : TEXT("false"),
		bIsTargetStealing ? TEXT("true") : TEXT("false"),
		bTargetCurrentlyStealing ? TEXT("true") : TEXT("false"),
		TargetDistance,
		bCanStartReportFlow ? TEXT("true") : TEXT("false")
	);
}
