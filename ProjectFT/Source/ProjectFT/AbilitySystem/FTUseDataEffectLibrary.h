#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectFT/Struct/FTItemUseStruct.h"
#include "FTUseDataEffectLibrary.generated.h"

class UAbilitySystemComponent;
class UFTGameplayAbility;
class UGameplayEffect;

/**
 * FTItemUseStruct의 GameplayEffect 적용 규칙을 아이템 GA 밖에서도 재사용하게 하는 공용 헬퍼.
 *
 * 데이터 출처는 관여하지 않는다. 아이템 DA, 함정 액터, AI 공격 설정이 각자 보유한
 * FTItemUseStruct를 넘기면 UseEffects와 EffectMagnitudes만 해석해 대상 ASC에 적용한다.
 */
UCLASS()
class PROJECTFT_API UFTUseDataEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FGameplayTag ResolveCooldownTag(const FTItemUseStruct& UseData);

	static int32 ApplyUseEffectsFromAbility(
		UFTGameplayAbility* Ability,
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		const FTItemUseStruct& UseData,
		const FGameplayAbilityTargetDataHandle* TargetData = nullptr);

	static int32 ApplyUseEffectsFromASC(
		UAbilitySystemComponent* SourceASC,
		UAbilitySystemComponent* TargetASC,
		const FTItemUseStruct& UseData,
		float Level = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "FT|UseData", meta = (DefaultToSelf = "SourceActor"))
	static int32 ApplyUseEffectsToActor(
		AActor* SourceActor,
		AActor* TargetActor,
		const FTItemUseStruct& UseData,
		float Level = 1.0f);

	static bool IsUseDataOnCooldown(UAbilitySystemComponent* ASC, const FTItemUseStruct& UseData);

	static bool ApplyCooldownFromASC(
		UAbilitySystemComponent* SourceASC,
		const FTItemUseStruct& UseData,
		TSubclassOf<UGameplayEffect> CooldownEffectClass,
		float Level = 1.0f);

	/**
	 * @brief 대상 캐릭터의 ASC에서 해당 UseData의 쿨다운 진행 상태(남은 시간 및 총 지속시간)를 조회합니다.
	 * @param OwnerActor : 대상 캐릭터 (플레이어)
	 * @param UseData : 조회할 아이템 사용 데이터
	 * @param OutTimeRemaining : 남은 쿨다운 시간 (초)
	 * @param OutDuration : 전체 쿨다운 시간 (초)
	 * @return 쿨다운이 진행 중이면 true
	 */
	UFUNCTION(BlueprintPure, Category = "FT|UseData", meta = (DefaultToSelf = "OwnerActor"))
	static bool GetItemCooldownProgress(
		const AActor* OwnerActor,
		const FTItemUseStruct& UseData,
		float& OutTimeRemaining,
		float& OutDuration);

private:
	static void ApplySetByCallerMagnitudes(FGameplayEffectSpecHandle EffectSpec, const FTItemUseStruct& UseData);
};
