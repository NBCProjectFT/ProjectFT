// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_Taser.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"

UFTGA_Taser::UFTGA_Taser()
{
	// 아이템 데이터의 UseAbility가 이 클래스를 가리키면, 캐릭터가 이 태그로 발동한다(범용 UseItem과 다른 태그라 서로 안 섞임).
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_UseTaser;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_Taser::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 적용할 효과·수치는 (범용 UseItem과 동일하게) 페이로드에서 읽어 ActiveUseData에 캐싱한다(베이스).
	// 트레이스/타깃 적용 '로직'만 이 GA의 고유 부분.
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!CacheActiveItem(TriggerEventData) || !Avatar || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	//여기서부터 이 GA의 고유로직입니다.
	// 시야 기준 정면 트레이스: 플레이어는 컨트롤러(=카메라) 시점, 그 외(AI 등)는 눈높이.
	FVector ViewLocation = Avatar->GetActorLocation();
	FRotator ViewRotation = Avatar->GetActorRotation();
	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}
		else
		{
			Avatar->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		}
	}

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceRange;
	FHitResult Hit;
	// 전용 Weapon 트레이스 채널(ECC_GameTraceChannel1, DefaultEngine.ini)로 트레이스한다.
	// 캐릭터 베이스가 '캡슐=Weapon Ignore'로 두므로 트레이스가 캡슐을 통과해 메시(CharacterMesh: QueryOnly + Weapon 기본 Block)에 맞는다
	// → 부위 정밀 판정 가능(Hit.BoneName). 월드/스태틱은 기본 Block이라 벽도 막는다.
	FCollisionQueryParams Params(FName(TEXT("FTTaser")), /*bTraceComplex=*/false, Avatar);
	const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_GameTraceChannel1, Params);
	if (UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location     = ViewLocation;          // 발사 연출을 트레이스 시작 지점(시야 원점)에서 낸다.
		CueParams.Normal       = ViewRotation.Vector(); // 발사 방향도 트레이스 방향과 일치시킨다(Avatar 정면이 아닌 시야 기준).
		CueParams.Instigator   = Avatar;
		CueParams.EffectCauser = Avatar;
		SourceASC->ExecuteGameplayCue(TAG_FT_GameplayCue_Taser_Start, CueParams);
	}

	
	//여기서부터 효과 적용하는 코드입니다.
	if (bHit && Hit.GetActor())
	{
		// 첫 적중 대상에게 아이템 데이터의 효과(들)를 적용(베이스 헬퍼).
		const FGameplayAbilityTargetDataHandle TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Hit.GetActor());
		ApplyUseEffects(Handle, ActorInfo, ActivationInfo, &TargetData);
		UE_LOG(LogTemp, Log, TEXT("[Taser] hit %s — applied %d effect(s)."), *Hit.GetActor()->GetName(), ActiveUseData.UseEffects.Num());

		// 적중 연출: "무엇을 적용"은 위(효과), "어떻게 보이나"는 GameplayCue로 분리.
		// 1회성·위치 기반이라 임팩트 지점/노멀을 파라미터로 실어 시전자(소유) ASC에서 Execute → GC_TaserHit Notify(BP)가 그린다.
		if (UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = Hit.ImpactPoint;
			CueParams.Normal = Hit.ImpactNormal;
			CueParams.Instigator = Avatar;
			CueParams.EffectCauser = Hit.GetActor();
			SourceASC->ExecuteGameplayCue(TAG_FT_GameplayCue_Taser_Hit, CueParams);
		}
		
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
