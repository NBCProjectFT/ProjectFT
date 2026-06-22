// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGA_Example.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Cooldown.h"

UFTGA_Example::UFTGA_Example()
{
	// =====================================================================================
	// [Advanced] 인스턴싱 / 네트워크 정책
	// =====================================================================================

	// InstancingPolicy : 실행 시 어빌리티를 어떻게 인스턴스화할지(구현에서 할 수 있는 일을 제한).
	//   - InstancedPerActor     : 액터당 인스턴스 1개. 활성 간 상태 유지, RPC 가능. 가장 일반적.
	//   - InstancedPerExecution : 실행마다 새 인스턴스. 상태 없음, 동시 다중 실행 가능.
	//   - NonInstanced          : 5.5부터 deprecated → 사용 금지.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// NetExecutionPolicy : 네트워크에서 어디서/어떻게 실행할지("예측 실행 / 서버에 요청 / 그냥 실행").
	//   - LocalPredicted : 클라가 즉시 예측 실행 + 서버 확인. 반응성이 좋아 가장 흔함.
	//   - LocalOnly      : 로컬 제어 측에서만 실행.
	//   - ServerInitiated: 서버가 시작.
	//   - ServerOnly     : 서버에서만 실행.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// NetSecurityPolicy : 클라이언트가 실행/종료 변경을 요청할 수 있는지에 대한 보호 수준.
	//   - ClientOrServer / ServerOnlyExecution / ServerOnlyTermination / ServerOnly
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	// ReplicationPolicy : (레거시) 인스턴스 자체를 모두에게 복제할지. NetExecutionPolicy로 충분하므로 보통 ReplicateNo.
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;

	// 서버측 어빌리티를 클라이언트측에서 취소할 수 있도록 허용할지(서버는 항상 클라를 취소 가능).
	bServerRespectsRemoteAbilityCancellation = false;

	// 이미 활성 중인 인스턴스 어빌리티를 재활성 시도하면, 종료 후 재실행할지.
	bRetriggerInstancedAbility = false;

	// [Input] 입력 press/release 이벤트를 항상 서버로 복제할지.
	bReplicateInputDirectly = false;

	// =====================================================================================
	// [Costs / Cooldowns] 비용 / 쿨다운 GameplayEffect (CommitAbility 시점에 적용됨)
	// =====================================================================================

	// CooldownGameplayEffectClass : 쿨다운 GE. Commit 시 적용되고 만료 전까지 재사용 불가.
	//   ※ 본 프로젝트의 UFTGE_Cooldown은 지속시간을 SetByCaller(Data.Cooldown)로 주입받으므로,
	//     실제로 사용하려면 UFTGA_UseItem처럼 ApplyCooldown()을 오버라이드해 매그니튜드를 넣어야 한다.
	//     (여기서는 "클래스를 지정하는 방법" 자체를 보여주기 위한 예시.)
	CooldownGameplayEffectClass = UFTGE_Cooldown::StaticClass();

	// CostGameplayEffectClass : 비용 GE(마나/스태미나 등). Commit 시 적용. 프로젝트에 비용 GE가 생기면 지정.
	//   CostGameplayEffectClass = UFTGE_StaminaCost::StaticClass();

	// =====================================================================================
	// [Tags] 태그 — 본 예제의 핵심. 아래 태그 "값"은 설명용이며, 실제로는 용도에 맞는 태그를 넣는다.
	//   (컨테이너들은 UGameplayAbility의 protected 멤버라 서브클래스 생성자에서 직접 접근 가능.)
	// =====================================================================================

	// (1) AssetTags (구 AbilityTags) : "이 어빌리티가 가진 태그"(정체성).
	//     다른 어빌리티의 Cancel/BlockAbilitiesWithTag가 이 태그를 기준으로 매칭한다.
	//     ★ 5.5+ deprecated: 'AbilityTags = ...' 직접 대입 대신 SetAssetTags()로만 설정.
	{
		FGameplayTagContainer AssetTags;
		AssetTags.AddTag(TAG_FT_State_UsingItem); // 예시 — 실제로는 'Ability.Item.Use' 같은 고유 식별 태그
		SetAssetTags(AssetTags);
	}

	// (2) CancelAbilitiesWithTag : 이 어빌리티가 활성될 때, AssetTag가 여기에 매칭되는 "다른 어빌리티를 취소".
	CancelAbilitiesWithTag.AddTag(TAG_FT_State_UsingItem);

	// (3) BlockAbilitiesWithTag : 이 어빌리티가 활성인 "동안", AssetTag가 여기에 매칭되는 어빌리티의 "활성을 차단".
	BlockAbilitiesWithTag.AddTag(TAG_FT_State_UsingItem);

	// (4) ActivationOwnedTags : 활성 "동안" 소유자(아바타)에게 부여하고 종료 시 자동 제거하는 태그.
	//     상태 표현이나 다른 로직의 조건으로 활용한다(예: 시전 중 표시).
	ActivationOwnedTags.AddTag(TAG_FT_State_UsingItem);

	// (5) ActivationRequiredTags : 소유자가 "이 태그를 모두" 가지고 있어야 활성 가능(비우면 제약 없음).
	//     예: ActivationRequiredTags.AddTag(TAG_Status_Alive);

	// (6) ActivationBlockedTags : 소유자가 "이 중 하나라도" 가지고 있으면 활성 차단.
	ActivationBlockedTags.AddTag(TAG_FT_Cooldown_ItemUse); // 예시 — 쿨다운 중 차단(보통은 CheckCooldown이 처리)

	// (7) Source*Tags : GameplayEffectContext의 "Source"(효과 시전 주체)에 대한 요구/차단 태그.
	//     예: SourceRequiredTags.AddTag(...);  /  SourceBlockedTags.AddTag(...);

	// (8) Target*Tags : 효과 "Target"(대상)에 대한 요구/차단 태그.
	//     예: TargetRequiredTags.AddTag(...);  /  TargetBlockedTags.AddTag(...);

	// =====================================================================================
	// [Triggers] 이벤트 기반 자동 발동(선택). 특정 GameplayEvent 태그나 소유 태그 변화로 자동 활성.
	// =====================================================================================
	//   FAbilityTriggerData Trigger;
	//   Trigger.TriggerTag = TAG_FT_State_UsingItem;
	//   Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent; // 또는 OwnedTagAdded/Present
	//   AbilityTriggers.Add(Trigger);
}

void UFTGA_Example::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// CommitAbility가 Cost/Cooldown GE를 실제로 적용한다(비용 부족 등으로 실패하면 활성 중단).
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// ... 여기서 실제 어빌리티 로직(연출/태스크/효과 적용 등)을 수행 ...

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
