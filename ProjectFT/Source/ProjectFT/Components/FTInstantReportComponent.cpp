#include "FTInstantReportComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectFT/AI/FTAIControllerBase.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Message/FTReportMessageLibrary.h"
#include "ProjectFT/Struct/FTCharacterAttackedPayloadStruct.h"

UFTInstantReportComponent::UFTInstantReportComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTInstantReportComponent::Initialize(AFTAIControllerBase* InOwnerController, UAISenseConfig_Sight* InSightConfig)
{
	OwnerController = InOwnerController;
	SightConfig = InSightConfig;
}

void UFTInstantReportComponent::StartListening()
{
	if (CharacterAttackedListenerHandle.IsValid())
	{
		return;
	}

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	CharacterAttackedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_CharacterAttacked,
		this,
		&ThisClass::OnCharacterAttacked
	);
}

void UFTInstantReportComponent::StopListening()
{
	if (!CharacterAttackedListenerHandle.IsValid())
	{
		return;
	}

	UGameplayMessageSubsystem::Get(this).UnregisterListener(CharacterAttackedListenerHandle);
	CharacterAttackedListenerHandle = FGameplayMessageListenerHandle();
}

void UFTInstantReportComponent::TickInstantReport()
{
	UpdateTargetState();
	TryReportObservedStealing();
}

void UFTInstantReportComponent::HandleTargetPerceptionUpdated(AActor* Actor)
{
	if (!IsPlayerActor(Actor))
	{
		return;
	}

	TargetActor = Actor;
	UpdateTargetState();
	TryReportObservedStealing();
}

void UFTInstantReportComponent::OnCharacterAttacked(FGameplayTag Channel, const FFTCharacterAttackedPayloadStruct& Payload)
{
	AActor* SuspectActor = ResolvePlayerActor(Payload.InstigatorActor);
	if (!IsPlayerActor(SuspectActor))
	{
		return;
	}

	if (Payload.TargetActor == (OwnerController ? OwnerController->GetPawn() : nullptr))
	{
		RequestInstantReport(SuspectActor, SuspectActor->GetActorLocation());
		return;
	}

	if (!CanWitnessActor(SuspectActor) || !CanWitnessActor(Payload.TargetActor))
	{
		return;
	}

	const FVector ReportLocation = Payload.TargetActor
		? Payload.TargetActor->GetActorLocation()
		: SuspectActor->GetActorLocation();
	RequestInstantReport(SuspectActor, ReportLocation);
}

void UFTInstantReportComponent::UpdateTargetState()
{
	bHasSeenTarget = IsActorVisibleBySight(TargetActor);
}

void UFTInstantReportComponent::TryReportObservedStealing()
{
	if (!TargetActor || !bHasSeenTarget || !IsTargetStealing(TargetActor))
	{
		return;
	}

	RequestInstantReport(TargetActor, TargetActor->GetActorLocation());
}

void UFTInstantReportComponent::RequestInstantReport(AActor* SuspectActor, const FVector& ReportLocation)
{
	if (!SuspectActor || bReportRequested || (bReportOnlyOnce && bHasReported) || !IsReportCooldownReady())
	{
		return;
	}

	TargetActor = SuspectActor;
	PendingReportLocation = ReportLocation;
	bReportRequested = true;

	if (bLogInstantReportDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[InstantReport] Report requested: Reporter=%s Target=%s Location=%s"),
			*GetNameSafe(OwnerController ? OwnerController->GetPawn() : nullptr),
			*GetNameSafe(SuspectActor),
			*ReportLocation.ToString()
		);
	}
}

bool UFTInstantReportComponent::BroadcastRequestedReport()
{
	if (!OwnerController || !bReportRequested || !TargetActor || (bReportOnlyOnce && bHasReported) || !IsReportCooldownReady())
	{
		return false;
	}

	bReportRequested = false;
	bHasReported = true;
	LastReportCompletedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	UFTReportMessageLibrary::BroadcastNPCReportMessage(
		OwnerController,
		TAG_FT_Event_NPCReportStarted,
		OwnerController->GetPawn(),
		TargetActor,
		PendingReportLocation,
		0.0f,
		0.0f);

	UFTReportMessageLibrary::BroadcastNPCReportCompleted(
		OwnerController,
		OwnerController->GetPawn(),
		TargetActor,
		PendingReportLocation,
		100.0f);

	if (bLogInstantReportDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[InstantReport] Instant report broadcast: Reporter=%s Target=%s Location=%s"),
			*GetNameSafe(OwnerController->GetPawn()),
			*GetNameSafe(TargetActor),
			*PendingReportLocation.ToString()
		);
	}

	return true;
}

AActor* UFTInstantReportComponent::ResolvePlayerActor(AActor* DamageCauser) const
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

bool UFTInstantReportComponent::IsPlayerActor(const AActor* Actor) const
{
	const APawn* TargetPawn = Cast<APawn>(Actor);
	return TargetPawn && TargetPawn->IsPlayerControlled();
}

bool UFTInstantReportComponent::IsTargetStealing(const AActor* Actor) const
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

bool UFTInstantReportComponent::CanWitnessActor(AActor* Actor) const
{
	return IsActorVisibleBySight(Actor);
}

bool UFTInstantReportComponent::IsActorVisibleBySight(AActor* Actor) const
{
	const APawn* ControlledPawn = OwnerController ? OwnerController->GetPawn() : nullptr;
	if (!OwnerController || !ControlledPawn || !Actor || !SightConfig)
	{
		return false;
	}

	const FVector ToActor = Actor->GetActorLocation() - ControlledPawn->GetActorLocation();
	if (ToActor.SizeSquared() > FMath::Square(SightConfig->SightRadius))
	{
		return false;
	}

	const FVector Forward = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector DirectionToActor = ToActor.GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, DirectionToActor);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
	if (AngleDegrees > SightConfig->PeripheralVisionAngleDegrees)
	{
		return false;
	}

	return OwnerController->LineOfSightTo(Actor);
}

bool UFTInstantReportComponent::IsReportCooldownReady() const
{
	if (LastReportCompletedTime <= -FLT_MAX * 0.5f)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	return CurrentTime - LastReportCompletedTime >= ReportCooldown;
}
