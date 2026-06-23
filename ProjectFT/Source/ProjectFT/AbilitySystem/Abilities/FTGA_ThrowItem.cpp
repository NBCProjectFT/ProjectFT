// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_ThrowItem.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

UFTGA_ThrowItem::UFTGA_ThrowItem()
{
	// 아이템 데이터의 UseAbility가 이 클래스면, 캐릭터가 이 태그로 발동한다(다른 use-GA와 안 섞이게 전용 태그).
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_ThrowItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);

	// 쿨다운이 필요하면 아이템 데이터의 CooldownSeconds만 채우면 된다(베이스가 ApplyCooldown으로 처리).
}

void UFTGA_ThrowItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 적용할 효과·수치는 (범용 UseItem과 동일하게) 페이로드에서 읽어 ActiveUseData에 캐싱한다(베이스). 던지기 '로직'만 이 GA 고유.
	const UFTItemDataAsset* ItemAsset = CacheActiveItem(TriggerEventData);
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!ItemAsset || !Avatar || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 시점(컨트롤러=카메라) 기준 발사 지점/방향. (테이저의 시점 계산과 동일 패턴)
	FVector ViewLocation = Avatar->GetActorLocation();
	FRotator ViewRotation = Avatar->GetActorRotation();
	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}
	}

	// ───────────────────────────────────────────────────────────────────────
	// ★ 여기가 이 어빌리티의 고유 로직 자리(=던지기). 지금은 placeholder.
	//   TODO: ItemAsset->ItemData.ItemMesh를 ViewLocation에서 ViewRotation 방향으로 투사체 스폰 후 ThrowSpeed로 발사.
	//         투사체가 적중하면 그쪽에서 ActiveUseData(UseEffects)를 대상에 적용(테이저의 ApplyUseEffects와 동일 역할).
	// ───────────────────────────────────────────────────────────────────────
	UE_LOG(LogTemp, Log, TEXT("[ThrowItem] '%s' threw item (speed=%.0f)."), *Avatar->GetName(), ThrowSpeed);

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
