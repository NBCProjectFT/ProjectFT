// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FTGE_BubbleTrap.generated.h"

/**
 * 비눗방울에 '갇힌' 상태 GameplayEffect(Duration). 스택이 임계치에 도달하면 UFTGA_BubbleStackTrap이 적용한다.
 *
 * 이 GE 하나가 갇힘 상태의 모든 것을 담는다(전용 컴포넌트/타이머 없이):
 *  - Duration          : 자동 해제 타이머(SetByCaller Data.Duration으로 주입).
 *  - GrantedTags       : State.Debuff.Bubble(식별) + State.Debuff.Immobilized(이동정지·차단) + State.Escapable(연타 탈출).
 *  - GameplayCue       : GameplayCue.State.Bubble — 액터를 감싸는 큰 구체(비주얼은 GC_Bubble Notify가 담당).
 *
 * 해제 경로 둘 다 "이 GE 제거"로 통일된다: 타이머 만료(자동) / 연타로 가득 참(UFTGA_EscapableDebuff가 제거).
 * 잡기 대칭 배타: 적용 조건에 "State.Captured 있으면 무시"를 두어 잡힌 중에는 걸리지 않는다.
 */
UCLASS()
class PROJECTFT_API UFTGE_BubbleTrap : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFTGE_BubbleTrap();
};
