// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Hostile.generated.h"

/**
 * "이 적용은 공격이다"를 표시하는 마커 GameplayEffect(Instant, 그 자체론 아무 효과 없음).
 * 실제 효과(데미지/스턴/슬로우/독/비눗방울 등)와 '함께' 대상에게 적용하면, 대상(AFTCharacterBase)이
 * 이 GE의 에셋 태그 Effect.Hostile을 보고 "공격당함"으로 간주해 Event.Character.Attacked를 발행한다(어그로 신호).
 *
 * 설계 의도: 공격성을 효과 클래스에 내재시키지 않고, '적용 지점'이 이 마커를 함께 적용할지로 선언한다.
 * 같은 효과라도 이 마커와 함께 적용하면 공격, 아니면 비공격(자가 버프/힐 등)으로 취급된다.
 * (적용은 UFTUseDataEffectLibrary의 bApplyHostileMarker 경로 또는 공격 어빌리티가 직접 담당한다.)
 */
UCLASS()
class PROJECTFT_API UFTGE_Hostile : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Hostile();
};
