// Fill out your copyright notice in the Description page of Project Settings.

#include "FTChanneledInteractionComponent.h"

#include "ProjectFT/Core/FTLogChannels.h"

UFTChanneledInteractionComponent::UFTChanneledInteractionComponent()
{
	// 채널링 중에만 틱한다(컴포넌트 자체 Tick — 소유 액터 Tick과 독립).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFTChanneledInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsChanneling)
	{
		TickChannel(DeltaTime);
	}
}

void UFTChanneledInteractionComponent::StartChannel(AActor* InInteractor)
{
	if (bIsChanneling)
	{
		return;
	}

	Interactor = InInteractor;
	bIsChanneling = true;
	SetComponentTickEnabled(true);
	ScheduleNextSkillCheck();

	OnChannelStateChanged.Broadcast(true);
}

void UFTChanneledInteractionComponent::StopChannel()
{
	if (!bIsChanneling)
	{
		return;
	}

	bIsChanneling = false;

	// 진행 중이던 스킬체크는 결과 없이 닫는다(중단 패널티는 두지 않음).
	if (bSkillCheckActive)
	{
		bSkillCheckActive = false;
		OnSkillCheckEnded.Broadcast(EFTSkillCheckResultType::None);
	}

	SetComponentTickEnabled(false);
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

	// 3) 진행도는 스킬체크 중에도 계속 증가한다(DBD와 동일).
	SetProgress(Progress + DeltaTime / RequiredSeconds);

	// 4) 완료 판정.
	if (Progress >= 1.0f)
	{
		CompleteChannel();
	}
}

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

void UFTChanneledInteractionComponent::EndSkillCheck(EFTSkillCheckResultType Result)
{
	bSkillCheckActive = false;

	switch (Result)
	{
	case EFTSkillCheckResultType::Great:
		SetProgress(Progress + GreatProgressBonus);
		break;

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
		// Good / None: 진행도 변화 없음.
		break;
	}

	UE_LOG(LogFTPlayer, Verbose, TEXT("SkillCheck result on '%s': %d (cursor=%.2f, target=%.2f)."),
		*GetNameSafe(GetOwner()), static_cast<int32>(Result), SkillCheckCursor, SkillCheckTarget);

	OnSkillCheckEnded.Broadcast(Result);
	ScheduleNextSkillCheck();
}

void UFTChanneledInteractionComponent::ScheduleNextSkillCheck()
{
	TimeUntilNextSkillCheck = FMath::FRandRange(SkillCheckIntervalMin, SkillCheckIntervalMax);
}

void UFTChanneledInteractionComponent::SetProgress(float NewProgress)
{
	// 연속값이라 UI는 GetProgress()를 매 프레임 폴링(UMG 바인딩)하는 것을 권장한다.
	Progress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
}

void UFTChanneledInteractionComponent::CompleteChannel()
{
	Progress = 1.0f;
	bIsChanneling = false;

	if (bSkillCheckActive)
	{
		bSkillCheckActive = false;
		OnSkillCheckEnded.Broadcast(EFTSkillCheckResultType::None);
	}

	SetComponentTickEnabled(false);
	Interactor = nullptr;

	UE_LOG(LogFTPlayer, Verbose, TEXT("Channeled interaction completed on '%s'."), *GetNameSafe(GetOwner()));

	OnChannelStateChanged.Broadcast(false);
	OnCompleted.Broadcast();
}
