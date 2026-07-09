// Fill out your copyright notice in the Description page of Project Settings.

#include "FTChanneledInteractionComponent.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

#include "ProjectFT/Core/FTLogChannels.h"

UFTChanneledInteractionComponent::UFTChanneledInteractionComponent()
{
	// 채널링 중에만 틱한다(컴포넌트 자체 Tick — 소유 액터 Tick과 독립).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bAutoActivate = true;
}

void UFTChanneledInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsChanneling)
	{
		TickChannel(DeltaTime);
	}
}

void UFTChanneledInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 채널 도중 대상(소유 액터)이 파괴되면 Stop/Complete를 못 거치므로, 시전자 ASC에 부여한 태그를 여기서 회수한다.
	// bIsChanneling일 때만 회수해 Add/Remove 균형을 유지한다(이미 정리됐으면 no-op).
	if (bIsChanneling)
	{
		ApplyChannelingStateTag(false);
	}

	Super::EndPlay(EndPlayReason);
}

void UFTChanneledInteractionComponent::ApplyChannelingStateTag(bool bApply)
{
	if (!ChannelingStateTag.IsValid() || !Interactor)
	{
		return;
	}

	// 시전자가 GAS를 쓰면(플레이어) ASC에 Loose 태그로 부여/회수한다. ASC가 없으면 무시.
	if (const IAbilitySystemInterface* AbilitySystemActor = Cast<IAbilitySystemInterface>(Interactor))
	{
		if (UAbilitySystemComponent* ASC = AbilitySystemActor->GetAbilitySystemComponent())
		{
			if (bApply)
			{
				ASC->AddLooseGameplayTag(ChannelingStateTag);
			}
			else
			{
				ASC->RemoveLooseGameplayTag(ChannelingStateTag);
			}
		}
	}
}

//플레이어가 진열대 털기 시작하는 시점. (채널링 시작.)
void UFTChanneledInteractionComponent::StartChannel(AActor* InInteractor, float InWorkSpeedMultiplier)
{
	if (bIsChanneling)
	{
		return;
	}

	if (!IsActive())
	{
		UE_LOG(LogFTPlayer, Warning, TEXT("StartChannel failed: ChanneledInteraction on %s is NOT active!"), *GetNameSafe(GetOwner()));
		return;
	}

	Interactor = InInteractor;
	WorkSpeedMultiplier = FMath::Max(0.0f, InWorkSpeedMultiplier);
	bIsChanneling = true;
	SetComponentTickEnabled(true);
	ScheduleNextSkillCheck();

	// 채널 시작 → 시전자 ASC에 상태 태그 부여(도둑질 채널이면 State.Stealing).
	ApplyChannelingStateTag(true);

	OnChannelStateChanged.Broadcast(true);
}

//플레이어가 상호작용 키에서 손을 떼는 경우. (채널링 멈춤.)
void UFTChanneledInteractionComponent::StopChannel()
{
	if (!bIsChanneling)
	{
		return;
	}

	bIsChanneling = false;

	// 진행 중이던 스킬체크는 결과 없이 닫는다(중단 패널티는 구현 안했음. 미스로 처리하려면 여기에 넣으시면 됩니다.)
	if (bSkillCheckActive)
	{
		bSkillCheckActive = false;
		OnSkillCheckEnded.Broadcast(EFTSkillCheckResultType::None);
	}

	SetComponentTickEnabled(false);

	// 태그 회수는 Interactor를 비우기 전에(회수 대상 ASC를 잃지 않도록).
	ApplyChannelingStateTag(false);
	Interactor = nullptr;

	OnChannelStateChanged.Broadcast(false);
}

void UFTChanneledInteractionComponent::NotifySkillCheckInput()
{
	// 활성 스킬체크가 있을 때만 판정. 없을 때 입력은 무시한다.
	if (!bIsChanneling || !bSkillCheckActive)
	{
		return;
	}

	EndSkillCheck(EvaluateSkillCheckAtCursor());
}

//진행도 증가, 스킬체크 돌리기, 스킬체크 간격 계산 등 틱으로 처리해야 하는 로직들.
void UFTChanneledInteractionComponent::TickChannel(float DeltaTime)
{
	// 1) 스킬체크가 떠 있으면 커서를 진행시키고, 입력 없이 끝까지 가면 Miss로 확정.
	if (bSkillCheckActive)
	{
		SkillCheckCursor += DeltaTime / SkillCheckSweepSeconds;
		if (SkillCheckCursor >= 1.0f)
		{
			SkillCheckCursor = 1.0f;
			EndSkillCheck(EFTSkillCheckResultType::Miss);
		}
	}
	else
	{
		// 2) 다음 스킬체크까지 카운트다운.
		TimeUntilNextSkillCheck -= DeltaTime;
		if (TimeUntilNextSkillCheck <= 0.0f)
		{
			StartSkillCheck();
		}
	}

	// 3) 진행도는 스킬체크 중에도 계속 증가한다(DBD와 동일). 손재주 등 작업 속도 배수를 곱한다.
	SetProgress(Progress + (DeltaTime / RequiredSeconds) * WorkSpeedMultiplier);

	// 4) 100% 되면 완료 판정.
	if (Progress >= 1.0f)
	{
		CompleteChannel();
	}
}

//스킬체크 시작. (UI상에 시각적으로 나타나는 지점이 여기)
void UFTChanneledInteractionComponent::StartSkillCheck()
{
	// 성공존이 0..1 안에 완전히 들어오도록 타깃 범위를 클램프한다(경계 잘림 방지 + 반응 시간 확보).
	const float Lo = FMath::Max(SkillCheckTargetMin, SkillCheckSuccessHalfWidth);
	const float Hi = FMath::Min(SkillCheckTargetMax, 1.0f - SkillCheckSuccessHalfWidth);
	SkillCheckTarget = (Lo <= Hi) ? FMath::FRandRange(Lo, Hi) : 0.5f;

	SkillCheckCursor = 0.0f;
	bSkillCheckActive = true;

	OnSkillCheckStarted.Broadcast();
}

//스킬체크 타겟 위치로부터 오차범위 계산해서 성공실패 등 따지는 함수(스킬체크 시 성공 영역의 너비를 계산한다고 보시면 됩니다.)
EFTSkillCheckResultType UFTChanneledInteractionComponent::EvaluateSkillCheckAtCursor() const
{
	const float Distance = FMath::Abs(SkillCheckCursor - SkillCheckTarget);

	if (SkillCheckGreatHalfWidth > 0.0f && Distance <= SkillCheckGreatHalfWidth)
	{
		return EFTSkillCheckResultType::Great;
	}
	if (Distance <= SkillCheckSuccessHalfWidth)
	{
		return EFTSkillCheckResultType::Good;
	}
	return EFTSkillCheckResultType::Miss;
}

//성공이든 실패든 스킬체크 끝나는 지점.
void UFTChanneledInteractionComponent::EndSkillCheck(EFTSkillCheckResultType Result)
{
	bSkillCheckActive = false;

	switch (Result)
	{
		//스킬체크 Great일 때 실행하고 싶은 로직은 여기 작성
	case EFTSkillCheckResultType::Great:
		SetProgress(Progress + GreatProgressBonus);
		break;

		//스킬체크 Miss일 때 실행하고 싶은 로직은 여기 작성
	case EFTSkillCheckResultType::Miss:
		SetProgress(Progress - MissProgressPenalty);
		if (bMissStopsChannel)
		{
			OnSkillCheckEnded.Broadcast(Result);
			StopChannel();
			return;
		}
		break;

	default:
		// Good / None: 진행도 변화 없음. 필요하면 위에 Great, Miss처럼 만드시면 됩니다.
		break;
	}

	UE_LOG(LogFTPlayer, Verbose, TEXT("SkillCheck result on '%s': %d (cursor=%.2f, target=%.2f)."),
		*GetNameSafe(GetOwner()), static_cast<int32>(Result), SkillCheckCursor, SkillCheckTarget);

	OnSkillCheckEnded.Broadcast(Result);
	ScheduleNextSkillCheck();
}

//다음 스킬체크 위치 어디에 뜨게 할지 랜덤 돌리는 함수.
void UFTChanneledInteractionComponent::ScheduleNextSkillCheck()
{
	TimeUntilNextSkillCheck = FMath::FRandRange(SkillCheckIntervalMin, SkillCheckIntervalMax);
}

void UFTChanneledInteractionComponent::SetProgress(float NewProgress)
{
	// 연속값이라 UI는 GetProgress()를 매 프레임 폴링(UMG 바인딩)하는 것을 권장한다.
	// Progress(진행도 비율)가 0~1 벗어나지 않게끔.
	Progress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
}


//진열대 털기 게이지 다 채웠을 때 완수 로직. 
void UFTChanneledInteractionComponent::CompleteChannel()
{
	Progress = 0.0f;
	bIsChanneling = false;

	if (bSkillCheckActive)
	{
		bSkillCheckActive = false;
		OnSkillCheckEnded.Broadcast(EFTSkillCheckResultType::None);
	}

	SetComponentTickEnabled(false);

	// 완료 시에도 태그 회수 후 Interactor 정리(대상이 완료 직후 Destroy돼도 태그가 남지 않도록).
	ApplyChannelingStateTag(false);
	Interactor = nullptr;

	UE_LOG(LogFTPlayer, Verbose, TEXT("Channeled interaction completed on '%s'."), *GetNameSafe(GetOwner()));

	OnChannelStateChanged.Broadcast(false);
	
	//여기가 실직적으로 완수 로직 발동시키는 곳. 실제 어떤 일이 벌어지는지는 FTLootTable액터에 구현 돼있습니다.
	OnCompleted.Broadcast();
}
