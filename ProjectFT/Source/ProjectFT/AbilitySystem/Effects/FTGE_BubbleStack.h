// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_BubbleStack.generated.h"

/**
 * 버블건 피격 '스택' GameplayEffect(Duration). 피격 1회당 이 GE 1개가 대상에게 적용되며,
 * 활성 동안 State.Debuff.BubbleStack 태그를 부여한다(적용마다 카운트 +1).
 *
 * 스택끼리 합쳐지지 않도록 스태킹을 두지 않는다 → 적용마다 독립 인스턴스가 되어 각자 짧은 지속 뒤 개별 만료한다.
 * 그래서 태그 카운트는 "최근 창(고정 지속) 안에 몇 발 맞았는지"를 나타내고, 시간이 지나면 자연히 줄어든다.
 * 실제 '갇힘' 전환(임계 카운트 판정)은 이 태그를 감시하는 UFTGA_BubbleStackTrap이 담당하고, 갇힘 상태(타이머/이동정지/비주얼)는 UFTGE_BubbleTrap이 담는다.
 */
UCLASS()
class PROJECTFT_API UFTGE_BubbleStack : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_BubbleStack();
};
