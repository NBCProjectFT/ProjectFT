#include "FTNPCReportComponent.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProjectFT/Character/FTAICharacterBase.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Message/FTReportMessageLibrary.h"
#include "ProjectFT/NPC/FTNPCAIController.h"
#include "ProjectFT/Struct/FTCharacterAttackedPayloadStruct.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

UFTNPCReportComponent::UFTNPCReportComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFTNPCReportComponent::EnterReporting()
{
	ResetReportState();

	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	Controller->UpdateTargetState();
	if (Controller->bLogReportDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[NPC] Enter Reporting"));
	}
	BroadcastReportMessage(TAG_FT_Event_NPCReportStarted, Controller->TargetActor, 0.0f, 0.0f);
	PlayReportMontage();

	if (ShouldCancelReport(Controller))
	{
		CancelReport();
	}
}

bool UFTNPCReportComponent::TickReporting(float DeltaTime)
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return false;
	}

	if (bReportCompleted)
	{
		return true;
	}

	if (bReportCancelled)
	{
		return false;
	}

	Controller->UpdateTargetState();
	if (ShouldCancelReport(Controller))
	{
		CancelReport();
		return false;
	}

	const bool bShouldIncreaseReport = Controller->bHasSeenTarget
		&& (Controller->bIsTargetActivelyStealing || bObservedShelfDamaged || bObservedAssault);

	// 신고 대상이 아직 시야 안에 있으면 의심을 유지하고, 시야 밖일 때만 게이지를 감소시킨다.
	if (!bShouldIncreaseReport && Controller->bHasSeenTarget)
	{
		return false;
	}

	if (!bShouldIncreaseReport)
	{
		if (CurrentReportProgress > 0.0f)
		{
			ReportElapsedTime = FMath::Max(
				ReportElapsedTime - DeltaTime * (ReportDuration / FMath::Max(ReportDecayDuration, KINDA_SMALL_NUMBER)),
				0.0f
			);
			CurrentReportProgress = FMath::Clamp(ReportElapsedTime / FMath::Max(ReportDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
			BroadcastReportMessage(TAG_FT_Event_NPCReportProgress, Controller->TargetActor, ReportAmount, CurrentReportProgress);

			const int32 ReportPercent = FMath::FloorToInt(CurrentReportProgress * 100.0f);
			if (ReportPercent / 10 < LastLoggedReportDecayPercent / 10)
			{
				LastLoggedReportDecayPercent = ReportPercent;
				if (Controller->bLogReportDebug)
				{
					UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Decay %d%%"), ReportPercent);
				}
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
	BroadcastReportMessage(TAG_FT_Event_NPCReportProgress, Controller->TargetActor, ReportAmount, CurrentReportProgress);
	LastLoggedReportDecayPercent = 101;

	const int32 ReportPercent = FMath::FloorToInt(CurrentReportProgress * 100.0f);
	if (ReportPercent / 10 > LastLoggedReportPercent / 10)
	{
		LastLoggedReportPercent = ReportPercent;
		if (Controller->bLogReportDebug)
		{
			UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Progress %d%%"), ReportPercent);
		}
	}

	if (CurrentReportProgress >= 1.0f)
	{
		CompleteReport();
		return true;
	}

	return false;
}

void UFTNPCReportComponent::CancelReport()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	if (bReportCompleted || bReportCancelled)
	{
		return;
	}

	bReportCancelled = true;
	bObservedShelfDamaged = false;
	bObservedAssault = false;
	CurrentReportProgress = 0.0f;
	ReportElapsedTime = 0.0f;
	LastLoggedReportDecayPercent = 0;
	BroadcastReportMessage(TAG_FT_Event_NPCReportProgress, Controller->TargetActor, ReportAmount, 0.0f);
	StopReportMontage();
	if (Controller->bLogReportDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Cancelled"));
	}
}

void UFTNPCReportComponent::HandleReportFlowAvailability(bool bCanStartReportFlow, bool bLogReportDebug)
{
	if (bReportCompleted && IsReportCooldownReady())
	{
		ResetReportState();
		if (bLogReportDebug)
		{
			UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Rearmed"));
		}
	}

	if (bReportCancelled && bCanStartReportFlow)
	{
		ResetReportState();
		if (bLogReportDebug)
		{
			UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Retry Ready"));
		}
	}
}

bool UFTNPCReportComponent::HandleShelfDamaged(const FFTMessagePayloadStruct& Payload)
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return false;
	}

	AActor* SuspectActor = ResolvePlayerActor(Payload.InstigatorActor);
	AActor* DamagedShelf = Payload.TargetActor;
	if (!SuspectActor)
	{
		// TODO: Shelf damage 발행 측에서 실제 플레이어를 InstigatorActor로 보장하면 이 fallback을 제거한다.
		SuspectActor = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	if (!IsPlayerActor(SuspectActor) || !DamagedShelf)
	{
		if (Controller->bLogReportDebug)
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
		return false;
	}

	if (Controller->bIsStunned)
	{
		return false;
	}

	Controller->TargetActor = SuspectActor;
	Controller->UpdateTargetState();

	const bool bCanSeeDamagedShelf = Controller->LineOfSightTo(DamagedShelf);
	if (!Controller->bHasSeenTarget || !bCanSeeDamagedShelf)
	{
		if (Controller->bLogReportDebug)
		{
			UE_LOG(
				LogFTNPC,
				Log,
				TEXT("[NPC] Shelf damage not witnessed: PlayerVisible=%s ShelfVisible=%s Player=%s Shelf=%s"),
				Controller->bHasSeenTarget ? TEXT("true") : TEXT("false"),
				bCanSeeDamagedShelf ? TEXT("true") : TEXT("false"),
				*GetNameSafe(SuspectActor),
				*GetNameSafe(DamagedShelf)
			);
		}
		return false;
	}

	MarkObservedShelfDamage();

	if (Controller->bLogReportDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[NPC] Observed shelf damage: Player=%s Shelf=%s"),
			*GetNameSafe(SuspectActor),
			*GetNameSafe(DamagedShelf)
		);
	}

	return true;
}

bool UFTNPCReportComponent::HandleObservedAssault(const FFTCharacterAttackedPayloadStruct& Payload)
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return false;
	}

	AActor* SuspectActor = ResolvePlayerActor(Payload.InstigatorActor);
	AActor* DamagedActor = Payload.TargetActor;

	// 폭행 목격 신고를 처리할 수 없는 상황이면 중단한다.
	// 신고 컴포넌트가 없다 || 스턴 상태이다 || 공격자가 플레이어가 아니다 || 맞은 대상이 없다.
	if (Controller->bIsStunned || !IsPlayerActor(SuspectActor) || !DamagedActor)
	{
		return false;
	}

	// 맞은 대상이 AI 캐릭터가 아니라면 폭행 목격 신고 대상으로 보지 않는다.
	if (!Cast<AFTAICharacterBase>(DamagedActor))
	{
		return false;
	}

	// 공격자(플레이어)를 TargetActor로 지정하고 시야/거리 상태를 갱신한다.
	Controller->TargetActor = SuspectActor;
	Controller->UpdateTargetState();

	const bool bCanSeeDamagedActor = Controller->LineOfSightTo(DamagedActor);

	// 손님 NPC가 공격자와 피해자를 둘 다 봤을 때만 폭행 목격으로 인정한다.
	if (!Controller->bHasSeenTarget || !bCanSeeDamagedActor)
	{
		if (Controller->bLogReportDebug)
		{
			UE_LOG(
				LogFTNPC,
				Log,
				TEXT("[NPC] Assault not witnessed: PlayerVisible=%s VictimVisible=%s Player=%s Victim=%s"),
				Controller->bHasSeenTarget ? TEXT("true") : TEXT("false"),
				bCanSeeDamagedActor ? TEXT("true") : TEXT("false"),
				*GetNameSafe(SuspectActor),
				*GetNameSafe(DamagedActor)
			);
		}
		return false;
	}

	MarkObservedAssault();

	if (Controller->bLogReportDebug)
	{
		UE_LOG(
			LogFTNPC,
			Log,
			TEXT("[NPC] Observed assault: Player=%s Victim=%s"),
			*GetNameSafe(SuspectActor),
			*GetNameSafe(DamagedActor)
		);
	}

	return true;
}

void UFTNPCReportComponent::MarkObservedShelfDamage()
{
	bObservedShelfDamaged = true;
}

void UFTNPCReportComponent::MarkObservedAssault()
{
	bObservedAssault = true;
}

void UFTNPCReportComponent::HandleStunStateChanged(bool bStunned)
{
	if (bStunned && CurrentReportProgress > 0.0f && !bReportCompleted)
	{
		CancelReport();
	}
}

bool UFTNPCReportComponent::CanStartReport() const
{
	return !bReportCompleted || IsReportCooldownReady();
}

AFTNPCAIController* UFTNPCReportComponent::GetNPCAIController() const
{
	return Cast<AFTNPCAIController>(GetOwner());
}

AActor* UFTNPCReportComponent::ResolvePlayerActor(AActor* DamageCauser) const
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

bool UFTNPCReportComponent::IsPlayerActor(const AActor* Actor) const
{
	const APawn* TargetPawn = Cast<APawn>(Actor);
	return TargetPawn && TargetPawn->IsPlayerControlled();
}

bool UFTNPCReportComponent::ShouldCancelReport(const AFTNPCAIController* Controller) const
{
	if (!Controller || Controller->bIsStunned)
	{
		return true;
	}

	if (!Controller->TargetActor)
	{
		return true;
	}

	if (bObservedShelfDamaged || bObservedAssault)
	{
		return false;
	}

	return Controller->TargetDistance > ReportCancelDistance;
}

void UFTNPCReportComponent::PlayReportMontage() const
{
	if (!ReportMontage)
	{
		return;
	}

	const AFTNPCAIController* Controller = GetNPCAIController();
	const ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	const float SafePlayRate = FMath::Max(ReportMontagePlayRate, KINDA_SMALL_NUMBER);
	AnimInstance->Montage_Play(ReportMontage, SafePlayRate);
}

void UFTNPCReportComponent::StopReportMontage() const
{
	if (!ReportMontage)
	{
		return;
	}

	const AFTNPCAIController* Controller = GetNPCAIController();
	const ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(ReportMontage))
	{
		return;
	}

	AnimInstance->Montage_Stop(ReportMontageBlendOutTime, ReportMontage);
}

void UFTNPCReportComponent::CompleteReport()
{
	AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	if (bReportCompleted || bReportCancelled)
	{
		return;
	}

	bReportCompleted = true;
	bObservedShelfDamaged = false;
	bObservedAssault = false;
	CurrentReportProgress = 1.0f;
	LastReportCompletedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	BroadcastReportMessage(TAG_FT_Event_NPCReportCompleted, Controller->TargetActor, ReportAmount, 1.0f);
	StopReportMontage();
	if (Controller->bLogReportDebug)
	{
		UE_LOG(LogFTNPC, Log, TEXT("[NPC] Report Completed"));
	}
}

void UFTNPCReportComponent::BroadcastReportMessage(FGameplayTag Channel, AActor* TargetActor, float InReportAmount, float ReportProgress) const
{
	const AFTNPCAIController* Controller = GetNPCAIController();
	if (!Controller)
	{
		return;
	}

	UFTReportMessageLibrary::BroadcastNPCReportMessage(
		Controller,
		Channel,
		Controller->GetPawn(),
		TargetActor,
		TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector,
		InReportAmount,
		ReportProgress);
}

void UFTNPCReportComponent::ResetReportState()
{
	ReportElapsedTime = 0.0f;
	CurrentReportProgress = 0.0f;
	bReportCompleted = false;
	bReportCancelled = false;
	LastLoggedReportPercent = -1;
	LastLoggedReportDecayPercent = 101;
}

bool UFTNPCReportComponent::IsReportCooldownReady() const
{
	if (LastReportCompletedTime <= -FLT_MAX * 0.5f)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	return CurrentTime - LastReportCompletedTime >= ReportCooldown;
}
