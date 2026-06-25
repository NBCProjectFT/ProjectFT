// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_MeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTMeleeDataAsset.h"

UFTGA_MeleeAttack::UFTGA_MeleeAttack()
{
	// 이 Ability는 실행 중에 상태를 들고 있어야 합니다.
	//
	// 예:
	// - ActiveItemData
	// - ActiveMeleeData
	// - HitActors
	// - bTraceActive
	//
	// NonInstanced Ability로 만들면 여러 Actor가 같은 CDO를 공유하게 되어
	// 실행 중 상태를 안전하게 보관할 수 없습니다.
	//
	// InstancedPerActor:
	// - Actor마다 Ability 인스턴스를 하나씩 가집니다.
	// - 이 Ability처럼 실행 중 상태를 보관해야 할 때 적합합니다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 이 Ability는 직접 TryActivateAbility로 실행하지 않고,
	// GameplayEvent를 통해 실행되도록 등록합니다.
	//
	// EquipmentComponent / QuickSlot 쪽에서:
	// ASC->HandleGameplayEvent(TAG_FT_Event_UseItem, &Payload)
	//
	// 를 호출하면, GAS가 이 Trigger 정보를 보고 Ability를 활성화합니다.
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_FT_Event_UseItem;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UFTGA_MeleeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// GameplayEvent Payload에서 ItemData를 꺼냅니다.
	//
	// EquipmentComponent 쪽에서 보낸 Payload 예:
	//
	// FGameplayEventData Payload;
	// Payload.EventTag = TAG_FT_Event_UseItem;
	// Payload.Instigator = GetOwner();
	// Payload.Target = GetOwner();
	// Payload.OptionalObject = ItemData;
	//
	// 여기서는 OptionalObject를 UFTItemDataAsset으로 Cast해서 사용합니다.
	ActiveItemData = TriggerEventData
		? const_cast<UFTItemDataAsset*>(Cast<UFTItemDataAsset>(TriggerEventData->OptionalObject))
		: nullptr;

	// 근접 공격 Ability이므로 ItemData가 UFTMeleeDataAsset인지 확인합니다.
	// 성공하면 ActiveMeleeData에 저장하고, 이후 MeleeAttackData를 사용합니다.
	ActiveMeleeData = Cast<UFTMeleeDataAsset>(ActiveItemData);

	// 한 번의 Ability 실행이 시작될 때마다 이전 타격 기록과 Trace 상태를 초기화합니다.
	HitActors.Reset();
	bTraceActive = false;

	UE_LOG(LogFTItem, Log, TEXT("MeleeAttack activated. Avatar=%s Item=%s MeleeData=%s"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		*GetNameSafe(ActiveItemData),
		*GetNameSafe(ActiveMeleeData));

	// CommitAbility:
	// - 비용 Cost
	// - 쿨다운 Cooldown
	// - 활성화 조건
	// - 차단 태그
	//
	// 등을 GAS 기준으로 확정합니다.
	//
	// 실패하면 공격을 시작하면 안 되므로 Ability를 취소 종료합니다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 현재 ItemData에서 근접 공격 데이터를 가져옵니다.
	const FFTMeleeAttackStruct* MeleeData = GetMeleeAttackData();

	// ItemData가 UFTMeleeDataAsset이 아니거나,
	// MeleeAttackData를 얻을 수 없으면 이 Ability로 처리할 수 없습니다.
	if (!MeleeData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack ended: ItemData is not UFTMeleeDataAsset. Ability=%s Item=%s"),
			*GetNameSafe(this),
			*GetNameSafe(ActiveItemData));

		// Commit 이후지만 실제 공격을 수행하지 못한 상태입니다.
		// 현재 코드는 취소가 아니라 정상 종료로 처리하고 있습니다.
		// 필요하면 마지막 인자를 true로 바꿔 취소 종료로 볼 수도 있습니다.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 근접 공격에서 재생할 몽타주가 없으면 공격을 진행할 수 없습니다.
	if (!MeleeData->AttackMontage)
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack ended: AttackMontage is not set. Ability=%s Item=%s"),
			*GetNameSafe(this),
			*GetNameSafe(ActiveItemData));

		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// GAS 전용 Montage 재생 AbilityTask를 생성합니다.
	//
	// 일반 AnimInstance->Montage_Play()를 쓰는 것보다 좋은 점:
	// - Ability 생명주기와 몽타주 생명주기를 연결할 수 있습니다.
	// - 몽타주 완료/중단/취소 이벤트를 받을 수 있습니다.
	// - Ability가 몽타주가 끝날 때까지 활성 상태를 유지할 수 있습니다.
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("MeleeAttackMontage"),
			MeleeData->AttackMontage,
			1.0f
		);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 몽타주가 정상 완료되면 Ability도 정상 종료합니다.
	MontageTask->OnCompleted.AddDynamic(this, &UFTGA_MeleeAttack::HandleMontageCompleted);

	// BlendOut도 완료와 같은 처리로 묶었습니다.
	//
	// 주의:
	// - 상황에 따라 OnBlendOut과 OnCompleted가 둘 다 호출될 수 있습니다.
	// - EndAbility가 중복 호출될 가능성이 있으면 별도 bool로 방지하는 구조를 추가할 수 있습니다.
	MontageTask->OnBlendOut.AddDynamic(this, &UFTGA_MeleeAttack::HandleMontageCompleted);

	// 몽타주가 다른 몽타주에 의해 끊기거나 강제로 중단되면 취소 종료합니다.
	MontageTask->OnInterrupted.AddDynamic(this, &UFTGA_MeleeAttack::HandleMontageInterrupted);

	// Ability가 취소되거나 MontageTask가 취소되면 취소 종료합니다.
	MontageTask->OnCancelled.AddDynamic(this, &UFTGA_MeleeAttack::HandleMontageInterrupted);

	// AbilityTask를 실제로 시작합니다.
	MontageTask->ReadyForActivation();
}

void UFTGA_MeleeAttack::StartMeleeTrace()
{
	// 새로운 공격 판정 구간이 시작되므로,
	// 이전에 맞은 대상 기록을 초기화합니다.
	//
	// 이 덕분에 공격 한 번의 판정 구간 안에서는 중복 타격을 막고,
	// 다음 공격에서는 다시 같은 대상을 때릴 수 있습니다.
	HitActors.Reset();

	// PerformMeleeTrace가 실제로 동작할 수 있도록 상태를 켭니다.
	bTraceActive = true;

	// NotifyBegin이 호출된 바로 그 프레임도 놓치지 않게 즉시 한 번 검사합니다.
	//
	// 예:
	// - 검이 빠르게 지나가는 모션
	// - NotifyBegin 다음 Tick까지 기다리면 이미 지나친 경우
	//
	// 이런 상황을 줄이기 위한 처리입니다.
	PerformMeleeTrace();
}

void UFTGA_MeleeAttack::PerformMeleeTrace()
{
	// Trace가 활성화되어 있지 않으면 아무것도 하지 않습니다.
	//
	// 일반적으로:
	// - NotifyBegin에서 StartMeleeTrace()로 true
	// - NotifyEnd에서 StopMeleeTrace()로 false
	if (!bTraceActive)
	{
		return;
	}

	const FFTMeleeAttackStruct* MeleeData = GetMeleeAttackData();
	if (!MeleeData)
	{
		return;
	}

	// 공격자의 Mesh와 Actor를 찾습니다.
	USkeletalMeshComponent* SourceMesh = ResolveSourceMesh();
	AActor* SourceActor = GetAvatarActorFromActorInfo();

	if (!SourceMesh || !SourceActor)
	{
		return;
	}

	UWorld* World = SourceActor->GetWorld();
	if (!World)
	{
		return;
	}

	// 캡슐 Trace에 필요한 값들입니다.
	//
	// Start:
	// - HitStartSocketName 위치
	//
	// End:
	// - HitEndSocketName 위치
	//
	// CapsuleHalfHeight:
	// - 캡슐의 HalfHeight
	//
	// CapsuleRotation:
	// - 캡슐의 방향
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	float CapsuleHalfHeight = 0.0f;
	FQuat CapsuleRotation = FQuat::Identity;

	// 소켓 위치를 기준으로 캡슐 정보를 만듭니다.
	// 실패하면 이번 프레임 판정은 스킵합니다.
	if (!BuildTraceCapsule(SourceMesh, Start, End, CapsuleHalfHeight, CapsuleRotation))
	{
		return;
	}

	// OverlapMultiByChannel은 캡슐의 중심 위치가 필요합니다.
	const FVector CapsuleCenter = (Start + End) * 0.5f;

	// QueryParams:
	// - SourceActor는 자기 자신이므로 무시합니다.
	// - false는 bTraceComplex입니다. 여기서는 단순 충돌을 사용합니다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeleeAttack), false, SourceActor);
	QueryParams.AddIgnoredActor(SourceActor);

	TArray<FOverlapResult> OverlapResults;

	// 실제 캡슐 오버랩 판정입니다.
	//
	// MeleeData->TraceChannel:
	// - 어떤 Collision Channel로 검사할지 DataAsset에서 정합니다.
	//
	// FCollisionShape::MakeCapsule:
	// - Radius와 HalfHeight를 기준으로 캡슐을 만듭니다.
	World->OverlapMultiByChannel(
		OverlapResults,
		CapsuleCenter,
		CapsuleRotation,
		MeleeData->TraceChannel,
		FCollisionShape::MakeCapsule(MeleeData->CapsuleRadius, CapsuleHalfHeight),
		QueryParams
	);

	// 오버랩된 Actor들을 순회합니다.
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();

		// 유효하지 않은 대상, 자기 자신, 이미 맞은 대상은 제외합니다.
		if (!TargetActor || TargetActor == SourceActor || HitActors.Contains(TargetActor))
		{
			continue;
		}

		// 이번 공격에서 이 대상은 이미 맞은 것으로 기록합니다.
		// 이후 NotifyTick에서 다시 감지되어도 효과를 중복 적용하지 않습니다.
		HitActors.Add(TargetActor);

		// ItemData.UseData.UseEffects를 대상에게 적용합니다.
		ApplyItemEffectsToTarget(TargetActor);
	}

	// 디버그 Draw가 켜져 있으면 현재 판정 캡슐을 화면에 표시합니다.
	//
	// 노란/주황 구:
	// - 시작/끝 소켓 위치 확인용
	//
	// 노란 선:
	// - Start와 End를 잇는 축 확인용
	//
	// 빨간 캡슐:
	// - 실제 오버랩 판정 범위 확인용
	if (MeleeData->bDrawDebug)
	{
		constexpr float DebugLifeTime = 0.12f;
		constexpr float DebugThickness = 2.0f;

		DrawDebugSphere(
			World,
			Start,
			MeleeData->CapsuleRadius,
			12,
			FColor::Yellow,
			false,
			DebugLifeTime,
			0,
			DebugThickness
		);

		DrawDebugSphere(
			World,
			End,
			MeleeData->CapsuleRadius,
			12,
			FColor::Orange,
			false,
			DebugLifeTime,
			0,
			DebugThickness
		);

		DrawDebugLine(
			World,
			Start,
			End,
			FColor::Yellow,
			false,
			DebugLifeTime,
			0,
			DebugThickness
		);

		DrawDebugCapsule(
			World,
			CapsuleCenter,
			CapsuleHalfHeight,
			MeleeData->CapsuleRadius,
			CapsuleRotation,
			FColor::Red,
			false,
			DebugLifeTime,
			0,
			DebugThickness
		);
	}
}

void UFTGA_MeleeAttack::StopMeleeTrace()
{
	// 더 이상 PerformMeleeTrace가 실제 판정을 수행하지 않게 합니다.
	bTraceActive = false;

	// 이번 공격 구간의 타격 기록을 초기화합니다.
	//
	// 다음 공격에서는 같은 대상을 다시 때릴 수 있어야 하므로 비웁니다.
	HitActors.Reset();
}

void UFTGA_MeleeAttack::HandleMontageCompleted()
{
	// 몽타주가 정상적으로 끝난 경우입니다.
	// bWasCancelled=false로 Ability를 종료합니다.
	EndMeleeAbility(false);
}

void UFTGA_MeleeAttack::HandleMontageInterrupted()
{
	// 몽타주가 중단/취소된 경우입니다.
	// bWasCancelled=true로 Ability를 종료합니다.
	EndMeleeAbility(true);
}

void UFTGA_MeleeAttack::EndMeleeAbility(bool bWasCancelled)
{
	// Ability가 끝날 때는 공격 판정이 켜져 있더라도 반드시 정리합니다.
	StopMeleeTrace();

	// GAS에게 이 Ability 실행이 끝났다고 알립니다.
	//
	// CurrentSpecHandle / CurrentActorInfo / CurrentActivationInfo:
	// - 현재 실행 중인 Ability 인스턴스가 가지고 있는 실행 정보입니다.
	//
	// bReplicateEndAbility = true:
	// - 종료 사실을 네트워크에 복제합니다.
	//
	// bWasCancelled:
	// - true이면 취소 종료
	// - false이면 정상 종료
	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		bWasCancelled
	);
}

const FFTMeleeAttackStruct* UFTGA_MeleeAttack::GetMeleeAttackData() const
{
	// ActiveMeleeData가 유효하면 그 안의 MeleeAttackData를 반환합니다.
	// 유효하지 않으면 nullptr을 반환합니다.
	return ActiveMeleeData ? &ActiveMeleeData->MeleeAttackData : nullptr;
}

USkeletalMeshComponent* UFTGA_MeleeAttack::ResolveSourceMesh() const
{
	// GAS ActorInfo에 SkeletalMeshComponent가 등록되어 있으면 우선 사용합니다.
	if (CurrentActorInfo && CurrentActorInfo->SkeletalMeshComponent.IsValid())
	{
		return CurrentActorInfo->SkeletalMeshComponent.Get();
	}

	// ActorInfo에 Mesh가 없다면 AvatarActor에서 SkeletalMeshComponent를 검색합니다.
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

bool UFTGA_MeleeAttack::BuildTraceCapsule(
	USkeletalMeshComponent* SourceMesh,
	FVector& OutStart,
	FVector& OutEnd,
	float& OutHalfHeight,
	FQuat& OutRotation) const
{
	if (!SourceMesh)
	{
		return false;
	}

	const FFTMeleeAttackStruct* MeleeData = GetMeleeAttackData();
	if (!MeleeData)
	{
		return false;
	}

	// Trace에 사용할 시작/끝 소켓이 실제 Mesh에 존재하는지 확인합니다.
	//
	// 하나라도 없으면 판정 위치를 만들 수 없으므로 이번 프레임 Trace를 스킵합니다.
	if (!SourceMesh->DoesSocketExist(MeleeData->HitStartSocketName)
		|| !SourceMesh->DoesSocketExist(MeleeData->HitEndSocketName))
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack trace skipped: socket missing. Mesh=%s Start=%s(%s) End=%s(%s)"),
			*GetNameSafe(SourceMesh),
			*MeleeData->HitStartSocketName.ToString(),
			SourceMesh->DoesSocketExist(MeleeData->HitStartSocketName) ? TEXT("Found") : TEXT("Missing"),
			*MeleeData->HitEndSocketName.ToString(),
			SourceMesh->DoesSocketExist(MeleeData->HitEndSocketName) ? TEXT("Found") : TEXT("Missing"));

		return false;
	}

	// 소켓의 월드 위치를 가져옵니다.
	OutStart = SourceMesh->GetSocketLocation(MeleeData->HitStartSocketName);
	OutEnd = SourceMesh->GetSocketLocation(MeleeData->HitEndSocketName);

	// 두 소켓 사이의 방향/길이를 계산합니다.
	const FVector CapsuleAxis = OutEnd - OutStart;

	// 시작점과 끝점이 거의 같으면 캡슐 방향을 만들 수 없으므로 실패 처리합니다.
	if (CapsuleAxis.IsNearlyZero())
	{
		return false;
	}

	// UE Capsule의 HalfHeight는 캡슐 전체 반높이입니다.
	//
	// 여기서는 두 소켓 사이 거리를 캡슐의 길이 기준으로 삼습니다.
	//
	// CapsuleAxis.Size() * 0.5f:
	// - Start와 End 사이 거리의 절반
	//
	// FMath::Max(..., CapsuleRadius):
	// - HalfHeight가 Radius보다 작으면 캡슐 모양이 이상해질 수 있으므로
	//   최소값을 Radius로 보정합니다.
	OutHalfHeight = FMath::Max(CapsuleAxis.Size() * 0.5f, MeleeData->CapsuleRadius);

	// 캡슐의 로컬 Z축이 Start -> End 방향을 향하도록 회전값을 만듭니다.
	//
	// DrawDebugCapsule과 CollisionShape 캡슐은 기본적으로 Z축 방향을 기준으로 세워집니다.
	// 그래서 MakeFromZ를 사용합니다.
	OutRotation = FRotationMatrix::MakeFromZ(CapsuleAxis.GetSafeNormal()).ToQuat();

	return true;
}

void UFTGA_MeleeAttack::ApplyItemEffectsToTarget(AActor* TargetActor) const
{
	if (!ActiveItemData || !TargetActor)
	{
		return;
	}

	// 공격자 ASC입니다.
	// 이 Ability를 실행한 Owner/Avatar 쪽 ASC입니다.
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	// 타격 대상 ASC입니다.
	// 대상이 AbilitySystemInterface를 구현했거나 ASC를 찾을 수 있어야 합니다.
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	// Source 또는 Target ASC가 없으면 GameplayEffect를 적용할 수 없습니다.
	if (!SourceASC || !TargetASC)
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack effect skipped: ASC missing. SourceASC=%s Target=%s TargetASC=%s"),
			SourceASC ? TEXT("Valid") : TEXT("Missing"),
			*GetNameSafe(TargetActor),
			TargetASC ? TEXT("Valid") : TEXT("Missing"));

		return;
	}

	const FTItemUseStruct& UseData = ActiveItemData->ItemData.UseData;

	// 적용할 GameplayEffect 목록이 비어 있으면 아무것도 적용하지 않습니다.
	if (UseData.UseEffects.IsEmpty())
	{
		UE_LOG(LogFTItem, Warning, TEXT("MeleeAttack effect skipped: UseEffects is empty. Item=%s Target=%s"),
			*GetNameSafe(ActiveItemData),
			*GetNameSafe(TargetActor));

		return;
	}

	// GameplayEffectContext는 효과 적용의 출처 정보를 담습니다.
	//
	// 예:
	// - 어떤 아이템에서 발생했는지
	// - 누가 Instigator인지
	// - 나중에 ExecutionCalculation이나 GameplayCue에서 참조 가능
	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();

	// SourceObject로 ItemData를 넣습니다.
	// 나중에 효과 처리 로직에서 어떤 아이템이 원인인지 확인할 수 있습니다.
	EffectContext.AddSourceObject(ActiveItemData);

	// Instigator 정보를 넣습니다.
	// 현재는 SourceActor를 Instigator와 EffectCauser 둘 다로 사용합니다.
	if (AActor* SourceActor = GetAvatarActorFromActorInfo())
	{
		EffectContext.AddInstigator(SourceActor, SourceActor);
	}

	// ItemData.UseData.UseEffects에 등록된 모든 GE를 대상에게 적용합니다.
	for (const TSubclassOf<UGameplayEffect>& EffectClass : UseData.UseEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		// GameplayEffectSpec 생성.
		//
		// GetAbilityLevel():
		// - 현재 Ability 레벨을 GE 레벨로 사용합니다.
		const FGameplayEffectSpecHandle EffectSpec =
			SourceASC->MakeOutgoingSpec(
				EffectClass,
				GetAbilityLevel(),
				EffectContext
			);

		if (!EffectSpec.IsValid())
		{
			continue;
		}

		// SetByCaller Magnitude를 Spec에 주입합니다.
		//
		// 예:
		// UseData.EffectMagnitudes:
		// - Data.Damage = 20
		// - Data.Knockback = 500
		//
		// GameplayEffect 안에서 같은 GameplayTag를 SetByCaller로 읽으면
		// 이 값을 데미지/넉백/스턴 시간 등에 사용할 수 있습니다.
		for (const TPair<FGameplayTag, float>& Magnitude : UseData.EffectMagnitudes)
		{
			EffectSpec.Data->SetSetByCallerMagnitude(
				Magnitude.Key,
				Magnitude.Value
			);
		}

		// 최종적으로 SourceASC가 TargetASC에게 GE를 적용합니다.
		SourceASC->ApplyGameplayEffectSpecToTarget(
			*EffectSpec.Data.Get(),
			TargetASC
		);

		UE_LOG(LogFTItem, Log, TEXT("MeleeAttack effect applied. Item=%s Target=%s Effect=%s"),
			*GetNameSafe(ActiveItemData),
			*GetNameSafe(TargetActor),
			*GetNameSafe(EffectClass));
	}
}