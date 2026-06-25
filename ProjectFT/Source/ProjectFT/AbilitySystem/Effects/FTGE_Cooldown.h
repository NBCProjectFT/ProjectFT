// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Cooldown.generated.h"

/**
 * 아이템 사용 어빌리티 공용 쿨다운 GameplayEffect(Duration). 어빌리티가 직접 적용하는 '타이머 태그 홀더'.
 * 지속시간은 고정하지 않고 어빌리티가 SetByCaller(Data.Cooldown)로 주입한다.
 * 부여 태그도 정적으로 두지 않는다 — 아이템별 독립 쿨다운을 위해 어빌리티(UFTGA_ItemAbility::ApplyCooldown)가
 * 아이템별 태그를 DynamicGrantedTags로 얹고, 차단은 호출측이 그 태그로 판정한다. 표준 CooldownGameplayEffectClass 경로 미사용.
 */
UCLASS()
class PROJECTFT_API UFTGE_Cooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Cooldown();
};
