
#include "FTNPCAIController.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "EngineUtils.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "ProjectFT/Components/FTNPCReportComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/NPC/FTShoppingPoint.h"
#include "ProjectFT/Struct/FTNPCReportPayloadStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Struct/FTCharacterDamagePayloadStruct.h"

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
	ConfigureSight(NPCPerceptionComponent, SightConfig, 1500.0f, 40.0f, 2.0f);

	NPCReportComponent = CreateDefaultSubobject<UFTNPCReportComponent>(TEXT("NPCReportComponent"));
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

	Super::EndPlay(EndPlayReason);
}


void AFTNPCAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateTargetState();
	UpdateShoppingLook(DeltaTime);
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
	ReleaseShoppingTarget();

	TArray<AFTShoppingPoint*> PreferredShoppingPoints;
	TArray<AFTShoppingPoint*> FallbackShoppingPoints;
	float PreferredTotalWeight = 0.0f;
	float FallbackTotalWeight = 0.0f;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AFTShoppingPoint> It(World); It; ++It)
		{
			AFTShoppingPoint* ShoppingPoint = *It;
			if (!ShoppingPoint || ShoppingPoint->SelectionWeight <= 0.0f)
			{
				continue;
			}

			FallbackShoppingPoints.Add(ShoppingPoint);
			FallbackTotalWeight += ShoppingPoint->SelectionWeight;

			if (ShoppingPoint->CanSelectPreferred())
			{
				PreferredShoppingPoints.Add(ShoppingPoint);
				PreferredTotalWeight += ShoppingPoint->SelectionWeight;
			}
		}
	}

	const TArray<AFTShoppingPoint*>& ShoppingPoints = PreferredShoppingPoints.IsEmpty()
		? FallbackShoppingPoints
		: PreferredShoppingPoints;
	const float TotalWeight = PreferredShoppingPoints.IsEmpty()
		? FallbackTotalWeight
		: PreferredTotalWeight;

	if (ShoppingPoints.IsEmpty() || TotalWeight <= 0.0f)
	{
		ShoppingTargetLocation = FVector::ZeroVector;
		ShoppingLookLocation = FVector::ZeroVector;
		ShoppingTargetAcceptanceRadius = 100.0f;
		bHasShoppingTarget = false;
		if (bLogShoppingDebug)
		{
			UE_LOG(LogFTNPC, Warning, TEXT("NPC AI: No available shopping point found"));
		}
		return false;
	}

	// 여유 슬롯이 있는 포인트를 우선 선택하고, 없으면 모든 포인트 중에서 가중치로 선택한다.
	AFTShoppingPoint* SelectedShoppingPoint = nullptr;
	float RandomWeight = FMath::FRandRange(0.0f, TotalWeight);
	for (AFTShoppingPoint* ShoppingPoint : ShoppingPoints)
	{
		RandomWeight -= ShoppingPoint->SelectionWeight;
		if (RandomWeight <= 0.0f)
		{
			SelectedShoppingPoint = ShoppingPoint;
			break;
		}
	}

	if (!SelectedShoppingPoint)
	{
		SelectedShoppingPoint = ShoppingPoints.Last();
	}

	CurrentShoppingPoint = SelectedShoppingPoint;
	SelectedShoppingPoint->Reserve();
	SelectedShoppingPoint->GetRandomShoppingLocation(this, ShoppingTargetLocation);
	ShoppingLookLocation = ShoppingTargetLocation + SelectedShoppingPoint->GetActorForwardVector() * 500.0f;
	ShoppingTargetAcceptanceRadius = SelectedShoppingPoint->AcceptanceRadius;
	bHasShoppingTarget = true;

	if (bLogShoppingDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("NPC AI: Picked shopping target %s at %s"),
			*SelectedShoppingPoint->GetName(),
			*ShoppingTargetLocation.ToString());
	}
	return true;
}

bool AFTNPCAIController::PickRandomWanderTarget()
{
	return PickRandomShoppingTarget();
}

void AFTNPCAIController::ReleaseShoppingTarget()
{
	if (CurrentShoppingPoint)
	{
		CurrentShoppingPoint->Release();
		CurrentShoppingPoint = nullptr;
	}

	// 다음 쇼핑 목적지로 이동할 때 이전 시선 보간 상태를 정리한다.
	bBlendShoppingLook = false;
	CurrentShoppingLookLocation = FVector::ZeroVector;
	DesiredShoppingLookLocation = FVector::ZeroVector;
	bHasShoppingTarget = false;
	if (!bUsingReportFocus)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void AFTNPCAIController::StartShoppingLook()
{
	if (!bHasShoppingTarget)
	{
		return;
	}

	// 현재 바라보는 방향에서 쇼핑 목표 방향으로 천천히 보간하기 위해 목표 지점만 저장한다.
	DesiredShoppingLookLocation = ShoppingLookLocation;
	if (CurrentShoppingLookLocation.IsNearlyZero())
	{
		if (const APawn* ControlledPawn = GetPawn())
		{
			CurrentShoppingLookLocation = ControlledPawn->GetActorLocation() + ControlledPawn->GetActorForwardVector() * 500.0f;
		}
		else
		{
			CurrentShoppingLookLocation = DesiredShoppingLookLocation;
		}
	}

	bBlendShoppingLook = true;
}

void AFTNPCAIController::UpdateShoppingLook(float DeltaTime)
{
	if (!bBlendShoppingLook || bUsingReportFocus)
	{
		return;
	}

	// Focus 지점을 바로 바꾸지 않고 보간해서 손님 NPC의 시선 전환이 갑자기 꺾이지 않게 한다.
	CurrentShoppingLookLocation = FMath::VInterpTo(
		CurrentShoppingLookLocation,
		DesiredShoppingLookLocation,
		DeltaTime,
		ShoppingLookInterpSpeed);

	SetFocalPoint(CurrentShoppingLookLocation, EAIFocusPriority::Gameplay);

	if (FVector::DistSquared(CurrentShoppingLookLocation, DesiredShoppingLookLocation) > FMath::Square(10.0f))
	{
		return;
	}

	CurrentShoppingLookLocation = DesiredShoppingLookLocation;
	SetFocalPoint(CurrentShoppingLookLocation, EAIFocusPriority::Gameplay);
	bBlendShoppingLook = false;
}

void AFTNPCAIController::UpdateReportFocus()
{
	const bool bShouldFocusReportTarget = TargetActor
		&& bHasSeenTarget
		&& CurrentReportProgress > 0.0f
		&& !bReportCompleted
		&& !bIsStunned;

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

void AFTNPCAIController::HandleStunStateChanged(bool bStunned)
{
	bIsStunned = bStunned;

	if (NPCReportComponent)
	{
		NPCReportComponent->HandleStunStateChanged(bIsStunned);
		SyncReportStateFromComponent();
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
	AActor* SuspectActor = ResolvePlayerActor(Payload.InstigatorActor);
	AActor* DamagedShelf = Payload.TargetActor;
	if (!SuspectActor)
	{
		// TODO: Shelf damage 발행 측에서 실제 플레이어를 InstigatorActor로 보장하면 이 fallback을 제거한다.
		SuspectActor = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	if (!IsPlayerActor(SuspectActor) || !DamagedShelf)
	{
		if (bLogReportDebug)
		{
			UE_LOG(
				LogFTNPC,
				Warning,
				TEXT("[NPC] Ignored shelf damage: Instigator=%s ResolvedPlayer=%s Shelf=%s"),
				*GetNameSafe(Payload.InstigatorActor),
				*GetNameSafe(SuspectActor),
				*GetNameSafe(DamagedShelf)
			);
		}
		return;
	}

	if (bIsStunned)
	{
		return;
	}

	TargetActor = SuspectActor;
	UpdateTargetState();

	const bool bCanSeeDamagedShelf = LineOfSightTo(DamagedShelf);
	if (!bHasSeenTarget || !bCanSeeDamagedShelf)
	{
		if (bLogReportDebug)
		{
			UE_LOG(
				LogFTNPC,
				Log,
				TEXT("[NPC] Shelf damage not witnessed: PlayerVisible=%s ShelfVisible=%s Player=%s Shelf=%s"),
				bHasSeenTarget ? TEXT("true") : TEXT("false"),
				bCanSeeDamagedShelf ? TEXT("true") : TEXT("false"),
				*GetNameSafe(SuspectActor),
				*GetNameSafe(DamagedShelf)
			);
		}
		return;
	}

	if (NPCReportComponent)
	{
		NPCReportComponent->MarkObservedShelfDamage();
	}
	SyncReportStateFromComponent();
	bCanStartReportFlow = true;

	if (bLogReportDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[NPC] Observed shelf damage: Player=%s Shelf=%s"),
			*GetNameSafe(SuspectActor),
			*GetNameSafe(DamagedShelf)
		);
	}
}

void AFTNPCAIController::OnCharacterDamaged(FGameplayTag Channel, const FFTCharacterDamagePayloadStruct& Payload)
{
	if (Payload.TargetActor == GetPawn())
	{
		if (!NPCReportComponent || NPCReportComponent->CurrentReportProgress <= 0.0f || NPCReportComponent->bReportCompleted)
		{
			return;
		}

		CancelReport();

		if (bLogReportDebug)
		{
			UE_LOG(
				LogFTNPC,
				Log,
				TEXT("[NPC] Report reset by damage: NPC=%s Instigator=%s Damage=%.1f"),
				*GetNameSafe(GetPawn()),
				*GetNameSafe(Payload.InstigatorActor),
				Payload.DamageAmount
			);
		}

		return;
	}

	AActor* SuspectActor = ResolvePlayerActor(Payload.InstigatorActor);
	AActor* DamagedActor = Payload.TargetActor;
	if (!NPCReportComponent || bIsStunned || !IsPlayerActor(SuspectActor) || !DamagedActor)
	{
		return;
	}

	if (!Cast<AFTAICharacterBase>(DamagedActor))
	{
		return;
	}

	TargetActor = SuspectActor;
	UpdateTargetState();

	const bool bCanSeeDamagedActor = LineOfSightTo(DamagedActor);
	if (!bHasSeenTarget || !bCanSeeDamagedActor)
	{
		if (bLogReportDebug)
		{
			UE_LOG(
				LogFTNPC,
				Log,
				TEXT("[NPC] Assault not witnessed: PlayerVisible=%s VictimVisible=%s Player=%s Victim=%s"),
				bHasSeenTarget ? TEXT("true") : TEXT("false"),
				bCanSeeDamagedActor ? TEXT("true") : TEXT("false"),
				*GetNameSafe(SuspectActor),
				*GetNameSafe(DamagedActor)
			);
		}
		return;
	}

	NPCReportComponent->MarkObservedAssault();
	SyncReportStateFromComponent();
	bCanStartReportFlow = true;

	if (bLogReportDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[NPC] Observed assault: Player=%s Victim=%s Damage=%.1f"),
			*GetNameSafe(SuspectActor),
			*GetNameSafe(DamagedActor),
			Payload.DamageAmount
		);
	}
}

void AFTNPCAIController::SyncReportStateFromComponent()
{
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

void AFTNPCAIController::DrawSightDebug() const
{
	DrawFlatSightDebug(SightConfig, FColor::Cyan, 1.5f);
}

void AFTNPCAIController::LogReportConditionDebug(bool bTargetCurrentlyStealing)
{
	if (!bLogReportDebug)
	{
		return;
	}

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
