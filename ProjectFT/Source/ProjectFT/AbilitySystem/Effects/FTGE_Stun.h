// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_Stun.generated.h"

/**
 * 스턴 디버프 GameplayEffect(Duration). 지속 동안 대상에게 State.Debuff.Stun 태그를 부여한다.
 * 지속시간은 아이템 데이터가 SetByCaller(Data.StunDuration)로 주입한다.
 * (이동/행동 차단 enforcement는 이 태그를 읽는 쪽 — 어빌리티 ActivationBlockedTags, 이동 로직 등 — 에서 처리.)
 */
UCLASS()
class PROJECTFT_API UFTGE_Stun : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_Stun();
};
