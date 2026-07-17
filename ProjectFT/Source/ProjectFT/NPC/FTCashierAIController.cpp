#include "FTCashierAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Components/StateTreeAIComponent.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Message/FTReportMessageLibrary.h"
#include "ProjectFT/Struct/FTCharacterAttackedPayloadStruct.h"

AFTCashierAIController::AFTCashierAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	CashierStateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("CashierStateTreeAIComponent"));
	CashierPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("CashierPerceptionComponent"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
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

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	CharacterAttackedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_CharacterAttacked,
		this,
		&ThisClass::OnCharacterAttacked
	);
}

void AFTCashierAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CharacterAttackedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(CharacterAttackedListenerHandle);
		CharacterAttackedListenerHandle = FGameplayMessageListenerHandle();
	}

	Super::EndPlay(EndPlayReason);
}

void AFTCashierAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTargetState();
	TryReportObservedStealing();
	DrawFlatSightDebug(SightConfig, FColor::Yellow, 1.5f);
}

void AFTCashierAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!IsPlayerActor(Actor))
	{
		return;
	}

	TargetActor = Actor;
	UpdateTargetState();
	TryReportObservedStealing();
}

void AFTCashierAIController::OnCharacterAttacked(FGameplayTag Channel, const FFTCharacterAttackedPayloadStruct& Payload)
{
	AActor* SuspectActor = ResolvePlayerActor(Payload.InstigatorActor);
	if (!IsPlayerActor(SuspectActor))
	{
		return;
	}

	if (Payload.TargetActor == GetPawn())
	{
		BroadcastInstantReport(SuspectActor, SuspectActor->GetActorLocation());
		return;
	}

	if (!CanWitnessActor(SuspectActor) || !CanWitnessActor(Payload.TargetActor))
	{
		return;
	}

	const FVector ReportLocation = Payload.TargetActor
		? Payload.TargetActor->GetActorLocation()
		: SuspectActor->GetActorLocation();
	BroadcastInstantReport(SuspectActor, ReportLocation);
}

void AFTCashierAIController::UpdateTargetState()
{
	bHasSeenTarget = IsActorVisibleBySight(TargetActor, SightConfig);
}

void AFTCashierAIController::TryReportObservedStealing()
{
	if (!TargetActor || !bHasSeenTarget || !IsTargetStealing(TargetActor))
	{
		return;
	}

	BroadcastInstantReport(TargetActor, TargetActor->GetActorLocation());
}

void AFTCashierAIController::BroadcastInstantReport(AActor* SuspectActor, const FVector& ReportLocation)
{
	if (!SuspectActor || (bReportOnlyOnce && bHasReported))
	{
		return;
	}

	bHasReported = true;

	UFTReportMessageLibrary::BroadcastNPCReportCompleted(this, GetPawn(), SuspectActor, ReportLocation, 100.0f);

	if (bLogCashierDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[Cashier] Instant report: Cashier=%s Target=%s Location=%s"),
			*GetNameSafe(GetPawn()),
			*GetNameSafe(SuspectActor),
			*ReportLocation.ToString()
		);
	}
}

AActor* AFTCashierAIController::ResolvePlayerActor(AActor* DamageCauser) const
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

bool AFTCashierAIController::IsPlayerActor(const AActor* Actor) const
{
	const APawn* TargetPawn = Cast<APawn>(Actor);
	return TargetPawn && TargetPawn->IsPlayerControlled();
}

bool AFTCashierAIController::IsTargetStealing(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	AActor* MutableActor = const_cast<AActor*>(Actor);
	const IAbilitySystemInterface* AbilitySystemActor = Cast<IAbilitySystemInterface>(MutableActor);
	const UAbilitySystemComponent* ASC = AbilitySystemActor ? AbilitySystemActor->GetAbilitySystemComponent() : nullptr;
	return ASC && ASC->HasMatchingGameplayTag(TAG_FT_State_Stealing);
}

bool AFTCashierAIController::CanWitnessActor(AActor* Actor) const
{
	return IsActorVisibleBySight(Actor, SightConfig);
}
