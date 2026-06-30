// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_ThrowItem.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

UFTGA_ThrowItem::UFTGA_ThrowItem()
{
	// 조준(활성) 중 소유자에게 상태 태그를 부여(ActivationOwnedTags) → "아이템 동작 진행 중?" 가드 질의에 쓰인다.
	ActivationOwnedTags.AddTag(TAG_FT_State_UsingItem);

	// 식별 AssetTag(조준형). CancelAbilities는 AssetTags를 매칭한다. .Aimed는 이동 취소(.Channeled) 대상이 아니라
	// "달리며 던지기"가 가능하고, 퀵슬롯 전환 시엔 부모 Ability.ItemUse 질의에 걸려 취소된다.
	{
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(TAG_FT_Ability_ItemUse_Aimed);
		SetAssetTags(AssetTags);
	}

	// 아이템 데이터의 UseAbility가 이 클래스면, 캐릭터가 이 태그로 발동한다(다른 use-GA와 안 섞이게 전용 태그).
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_ThrowItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);

	// 쿨다운이 필요하면 아이템 데이터의 CooldownSeconds만 채우면 된다(베이스가 ApplyCooldown으로 처리, 커밋은 PerformThrow에서).
}

void UFTGA_ThrowItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 적용할 효과·수치는 (범용 UseItem과 동일하게) 페이로드에서 읽어 ActiveUseData에 캐싱한다(베이스). 던지기 '로직'만 이 GA 고유.
	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	if (!ItemAsset || !GetAvatarActorFromActorInfo())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 누르는 동안 조준한다(궤적 프리뷰는 BP가 OnAimStarted~OnAimStopped 사이에서 그린다).
	// 비용/쿨다운은 여기서 커밋하지 않는다 — 실제로 던지는 순간(PerformThrow)에 커밋해, 조준만 하다 취소하면 소모되지 않게 한다.
	OnAimStarted();

	// 사용 입력에서 손을 뗀 순간을 기다린다(Event.UseReleased = 비-트리거 제네릭 이벤트). 받으면 실제 투척.
	UAbilityTask_WaitGameplayEvent* ReleaseTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TAG_FT_Event_UseReleased, nullptr, /*OnlyTriggerOnce=*/true, /*OnlyMatchExact=*/true);
	if (!ReleaseTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	ReleaseTask->EventReceived.AddDynamic(this, &UFTGA_ThrowItem::HandleReleaseEvent);
	ReleaseTask->ReadyForActivation();
}

void UFTGA_ThrowItem::HandleReleaseEvent(FGameplayEventData Payload)
{
	PerformThrow();
}

void UFTGA_ThrowItem::PerformThrow()
{
	// 실제로 던지는 순간에 비용/쿨다운을 커밋(효과 적용 "전"에 → 비용 부족이면 던지지 않고 취소).
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 시점(컨트롤러=카메라) 기준 발사 지점/방향.
	FVector LaunchLocation;
	FRotator LaunchRotation;
	GetThrowLaunchPoint(LaunchLocation, LaunchRotation);

	// ───────────────────────────────────────────────────────────────────────
	// ★ 여기가 이 어빌리티의 고유 로직 자리(=던지기). 지금은 placeholder.
	//   TODO: ItemAsset->ItemData.ItemMesh를 LaunchLocation에서 LaunchRotation 방향으로 투사체 스폰 후 ThrowSpeed로 발사.
	//         투사체가 적중하면 그쪽에서 ActiveUseData(UseEffects)를 대상에 적용(테이저의 ApplyUseEffects와 동일 역할).
	// ───────────────────────────────────────────────────────────────────────
	UE_LOG(LogTemp, Log, TEXT("[ThrowItem] threw item (speed=%.0f)."), ThrowSpeed);

	// 던졌으니 소비를 알린다(인벤토리 차감 등은 메시지 리스너가 처리, 베이스 헬퍼).
	OnItemConsumed();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}

void UFTGA_ThrowItem::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 정상 종료/취소/중단 어디서 끝나든 조준 프리뷰를 정리한다(프리뷰는 어빌리티 수명에 묶임).
	OnAimStopped();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UFTGA_ThrowItem::GetThrowLaunchPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	OutLocation = Avatar->GetActorLocation();
	OutRotation = Avatar->GetActorRotation();
	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			Controller->GetPlayerViewPoint(OutLocation, OutRotation);
		}
	}
}
