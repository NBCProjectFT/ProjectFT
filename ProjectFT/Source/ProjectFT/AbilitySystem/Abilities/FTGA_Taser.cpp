// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_Taser.h"

#include "AbilitySystemBlueprintLibrary.h"
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
	// 트레이스 채널은 프로젝트 콜리전 설정에 맞춰 조정 가능(폰만 맞히려면 ECC_Pawn 등). 우선 가시성 채널로 '정면 첫 충돌'을 잡는다.
	FCollisionQueryParams Params(FName(TEXT("FTTaser")), /*bTraceComplex=*/false, Avatar);
	const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params);

	
	//여기서부터 효과 적용하는 코드입니다.
	if (bHit && Hit.GetActor())
	{
		// 첫 적중 대상에게 아이템 데이터의 효과(들)를 적용(베이스 헬퍼).
		const FGameplayAbilityTargetDataHandle TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Hit);
		ApplyUseEffects(Handle, ActorInfo, ActivationInfo, &TargetData);
		UE_LOG(LogTemp, Log, TEXT("[Taser] hit %s — applied %d effect(s)."), *Hit.GetActor()->GetName(), ActiveUseData.UseEffects.Num());
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
