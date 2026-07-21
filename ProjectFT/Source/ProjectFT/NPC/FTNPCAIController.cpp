
#include "FTNPCAIController.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Components/FTNPCReportComponent.h"
#include "ProjectFT/Components/FTNPCReactionComponent.h"
#include "ProjectFT/Components/FTNPCShoppingComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Struct/FTCharacterDamagePayloadStruct.h"
#include "ProjectFT/Struct/FTCharacterAttackedPayloadStruct.h"

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
	
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	ConfigureSight(NPCPerceptionComponent, SightConfig, 1000.0f, 90.0f, 2.0f);

	NPCReportComponent = CreateDefaultSubobject<UFTNPCReportComponent>(TEXT("NPCReportComponent"));
	NPCReactionComponent = CreateDefaultSubobject<UFTNPCReactionComponent>(TEXT("NPCReactionComponent"));
	NPCShoppingComponent = CreateDefaultSubobject<UFTNPCShoppingComponent>(TEXT("NPCShoppingComponent"));
}	


void AFTNPCAIController::BeginPlay()
{
	Super::BeginPlay();

	RefreshSightConfig(NPCPerceptionComponent, SightConfig);

	if (NPCPerceptionComponent)
	{
		NPCPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AFTNPCAIController::OnTargetPerceptionUpdated);
	}
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	ShelfDamagedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_ShelfDamaged,
		this,
		&ThisClass::OnShelfDamaged
	);
	CharacterDamagedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_CharacterDamaged,
		this,
		&ThisClass::OnCharacterDamaged
	);
	CharacterAttackedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_CharacterAttacked,
		this,
		&ThisClass::OnCharacterAttacked
	);
}

void AFTNPCAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseShoppingTarget();

	if (ShelfDamagedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(ShelfDamagedListenerHandle);
	}

	if (CharacterDamagedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(CharacterDamagedListenerHandle);
	}
	if (CharacterAttackedListenerHandle.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(CharacterAttackedListenerHandle);
	}

	Super::EndPlay(EndPlayReason);
}


void AFTNPCAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTargetState();
	if (NPCReactionComponent)
	{
		NPCReactionComponent->TickReaction();
	}
	if (NPCShoppingComponent)
	{
		NPCShoppingComponent->TickShoppingLook(DeltaTime);
	}
	UpdateReportFocus();
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
		if (bLogPerceptionDebug)
		{
			UE_LOG(LogFTNPC, Log, TEXT("NPC AI: Target sensed %s"), *GetNameSafe(Actor));
		}
		return;
	}

	if (Actor == TargetActor)
	{
		UpdateTargetState();
		if (bLogPerceptionDebug)
		{
			UE_LOG(LogFTNPC, Log, TEXT("NPC AI: Target lost %s"), *GetNameSafe(Actor));
		}
	}
}

bool AFTNPCAIController::PickRandomShoppingTarget()
{
	return NPCShoppingComponent
		? NPCShoppingComponent->PickRandomShoppingTarget()
		: false;
}

bool AFTNPCAIController::PickRandomWanderTarget()
{
	return PickRandomShoppingTarget();
}

void AFTNPCAIController::ReleaseShoppingTarget()
{
	if (NPCShoppingComponent)
	{
		NPCShoppingComponent->ReleaseShoppingTarget();
	}
}

void AFTNPCAIController::StartShoppingLook()
{
	if (NPCShoppingComponent)
	{
		NPCShoppingComponent->StartShoppingLook();
	}
}

void AFTNPCAIController::UpdateReportFocus()
{
	const bool bShouldFocusReportTarget = TargetActor
		&& bHasSeenTarget
		&& CurrentReportProgress > 0.0f
		&& !bReportCompleted
		&& !bIsStunned
		&& !bFleeRequested
		&& !bPanicRequested
		&& !bKnockedOut;

	if (bShouldFocusReportTarget)
	{
		// 신고 게이지가 남아 있고 대상이 보이면 의심 대상을 계속 응시한다.
		SetFocus(TargetActor, EAIFocusPriority::Gameplay);
		bUsingReportFocus = true;
		return;
	}

	if (bUsingReportFocus)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		bUsingReportFocus = false;
	}
}

void AFTNPCAIController::ClearReactionFocusState()
{
	if (NPCShoppingComponent)
	{
		NPCShoppingComponent->ClearShoppingFocusState();
	}
	bUsingReportFocus = false;
}

bool AFTNPCAIController::IsUsingReportFocus() const
{
	return bUsingReportFocus;
}

void AFTNPCAIController::EnterSuspicious()
{
	ReleaseShoppingTarget();
	UpdateTargetState();
	if (bLogReportDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[NPC] Enter Suspicious"));
	}
}

void AFTNPCAIController::EnterReporting()
{
	if (NPCReportComponent)
	{
		NPCReportComponent->EnterReporting();
		SyncReportStateFromComponent();
	}
}

bool AFTNPCAIController::TickReporting(float DeltaTime)
{
	if (!NPCReportComponent)
	{
		return false;
	}

	const bool bResult = NPCReportComponent->TickReporting(DeltaTime);
	SyncReportStateFromComponent();
	return bResult;
}

void AFTNPCAIController::CancelReport()
{
	if (NPCReportComponent)
	{
		NPCReportComponent->CancelReport();
		SyncReportStateFromComponent();
	}
}

void AFTNPCAIController::EnterPanic()
{
	if (NPCReactionComponent)
	{
		NPCReactionComponent->EnterPanic();
	}
}

void AFTNPCAIController::HandleStunStateChanged(bool bStunned)
{
	if (NPCReactionComponent)
	{
		NPCReactionComponent->HandleImmobilizedStateChanged(bStunned);
	}
	else
	{
		bIsStunned = bStunned;
	}

	if (NPCReportComponent)
	{
		NPCReportComponent->HandleStunStateChanged(bIsStunned);
		SyncReportStateFromComponent();
	}
}

bool AFTNPCAIController::PickFleeLocationFrom(AActor* ThreatActor)
{
	return NPCReactionComponent
		? NPCReactionComponent->PickFleeLocationFrom(ThreatActor)
		: false;
}

bool AFTNPCAIController::RequestFleeFromTarget()
{
	return NPCReactionComponent
		? NPCReactionComponent->RequestFleeFromTarget()
		: false;
}

void AFTNPCAIController::FinishFlee()
{
	if (NPCReactionComponent)
	{
		NPCReactionComponent->FinishFlee();
	}
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
		SyncReportStateFromComponent();
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
		SyncReportStateFromComponent();
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
	const bool bHasObservedShelfDamaged = NPCReportComponent && NPCReportComponent->bObservedShelfDamaged;
	const bool bHasObservedAssault = NPCReportComponent && NPCReportComponent->bObservedAssault;
	bCanStartReportFlow = !bIsStunned && TargetActor &&
		((bHasSeenTarget && bIsTargetStealing) || bHasObservedShelfDamaged || bHasObservedAssault);

	if (NPCReportComponent)
	{
		NPCReportComponent->HandleReportFlowAvailability(bCanStartReportFlow, bLogReportDebug);
		SyncReportStateFromComponent();
	}

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

AActor* AFTNPCAIController::ResolvePlayerActor(AActor* DamageCauser) const
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

bool AFTNPCAIController::IsTargetCurrentlyVisible() const
{
	return IsActorVisibleBySight(TargetActor, SightConfig);
}

bool AFTNPCAIController::IsTargetStealing(const AActor* Actor) const
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

void AFTNPCAIController::OnShelfDamaged(
	FGameplayTag Channel,
	const FFTMessagePayloadStruct& Payload)
{
	if (NPCReportComponent && NPCReportComponent->HandleShelfDamaged(Payload))
	{
		if (NPCReactionComponent)
		{
			NPCReactionComponent->PlaySurprisedMontage();
		}
		SyncReportStateFromComponent();
		bCanStartReportFlow = true;
	}
}

void AFTNPCAIController::OnCharacterDamaged(FGameplayTag Channel, const FFTCharacterDamagePayloadStruct& Payload)
{
	if (Payload.TargetActor != GetPawn() || !Payload.bTargetKnockedOut)
	{
		return;
	}

	bKnockedOut = true;
	bFleeRequested = false;
	bPanicRequested = false;

	if (bLogReportDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[NPC] Knocked out: NPC=%s Instigator=%s Damage=%.1f"),
			*GetNameSafe(GetPawn()),
			*GetNameSafe(Payload.InstigatorActor),
			Payload.DamageAmount
		);
	}
}

void AFTNPCAIController::OnCharacterAttacked(FGameplayTag Channel, const FFTCharacterAttackedPayloadStruct& Payload)
{
	if (Payload.TargetActor == GetPawn())
	{
		AActor* SuspectActor = ResolvePlayerActor(Payload.InstigatorActor);
		if (!IsPlayerActor(SuspectActor))
		{
			return;
		}

		bool bHasImmobilizeTag = Payload.EffectTags.HasTag(TAG_FT_State_Debuff_Immobilized);

		if (!bHasImmobilizeTag)
		{
			if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Payload.TargetActor))
			{
				if (UAbilitySystemComponent* TargetASC = AbilitySystemInterface->GetAbilitySystemComponent())
				{
					// Hostile Payload에 상태 태그가 없더라도 이미 적용된 행동불능 태그가 있으면 즉시 도망치지 않는다.
					bHasImmobilizeTag = TargetASC->HasMatchingGameplayTag(TAG_FT_State_Debuff_Immobilized);
				}
			}
		}

		TargetActor = SuspectActor;
		if (NPCReactionComponent)
		{
			NPCReactionComponent->SetLastThreatActor(SuspectActor);
			NPCReactionComponent->PlayReactingMontage();
		}

		if (NPCReportComponent && NPCReportComponent->CurrentReportProgress > 0.0f && !NPCReportComponent->bReportCompleted)
		{
			CancelReport();
		}

		if (!bHasImmobilizeTag && !bKnockedOut && NPCReactionComponent)
		{
			bFleeRequested = NPCReactionComponent->PickFleeLocationFrom(SuspectActor);
			if (bFleeRequested)
			{
				NPCReactionComponent->RequestFleeFromTarget();
			}
			bPanicRequested = false;
		}

		if (bLogReportDebug)
		{
			UE_LOG(
				LogFTNPC,
				Log,
				TEXT("[NPC] Attacked by player: NPC=%s Player=%s Immobilized=%s FleeRequested=%s PanicRequested=%s"),
				*GetNameSafe(GetPawn()),
				*GetNameSafe(SuspectActor),
				bHasImmobilizeTag ? TEXT("true") : TEXT("false"),
				bFleeRequested ? TEXT("true") : TEXT("false"),
				bPanicRequested ? TEXT("true") : TEXT("false")
			);
		}

		return;
	}

	if (NPCReportComponent && NPCReportComponent->HandleObservedAssault(Payload))
	{
		if (NPCReactionComponent)
		{
			NPCReactionComponent->PlaySurprisedMontage();
		}
		SyncReportStateFromComponent();
		bCanStartReportFlow = true;
	}
}

// 신고컴포넌트의 상태를 AIController쪽 변수로 복사합니다.
void AFTNPCAIController::SyncReportStateFromComponent()
{
	// 방어코드. 신고 컴포넌트 없으면 종료
	if (!NPCReportComponent)
	{
		return;
	}

	CurrentReportProgress = NPCReportComponent->CurrentReportProgress;
	bReportCompleted = NPCReportComponent->bReportCompleted;
	bReportCancelled = NPCReportComponent->bReportCancelled;
	bObservedShelfDamaged = NPCReportComponent->bObservedShelfDamaged;
	bObservedAssault = NPCReportComponent->bObservedAssault;
}

// 손님 NPC 시야 표시용
void AFTNPCAIController::DrawSightDebug() const
{
	DrawFlatSightDebug(SightConfig, FColor::Cyan, 1.5f);
}

// 손님 신고 조건 로그 표시용
void AFTNPCAIController::LogReportConditionDebug(bool bTargetCurrentlyStealing)
{
	if (!bLogReportDebug)
	{
		return;
	}

	// 이전 상태와 지금 상태 비교 후 같으면 로그 찍지 않음
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
