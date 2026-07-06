// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "FTGameplayAbility.h"
#include "ProjectFT/Struct/FTStruggleGaugeStruct.h"
#include "FTGA_EscapableDebuff.generated.h"

/**
 * '연타로 탈출하는' 자가 상태이상(비눗방울/빙결 등)의 공용 탈출 어빌리티.
 *
 * 트리거: State.Escapable이 붙는 순간(OwnedTagPresent) 자동 발동. 활성 동안 Event.Struggle(플레이어 발버둥)을 받아
 * 공용 게이지를 채우고, 가득 차면 State.Escapable을 부여한 GE(들)를 제거해 상태를 해제한다.
 * 자동 해제(타이머)는 그 GE의 Duration이 담당하며, 만료되면 State.Escapable이 사라져 이 어빌리티도 자동 종료된다.
 *
 * 효과마다 새 클래스를 만들 필요 없이, 이 하나가 데이터(제거 대상 = Escapable을 부여한 GE)로 모든 탈출형을 처리한다.
 * 잡기(두-바디)는 자체 컴포넌트가 처리하므로, 잡힌 중(State.Captured)에는 이 어빌리티가 활성화되지 않는다.
 */
UCLASS()
class PROJECTFT_API UFTGA_EscapableDebuff : public UFTGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGA_EscapableDebuff();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 탈출에 필요한 총 struggle(좌우 연타 누적). 갇힘 시작 시 공용 게이지에 주입된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Escape", meta = (ClampMin = "0.0"))
	float EscapeThreshold = 12.0f;

	// 연타 탈출 게이지(능동 GainPerFlip). 자가 상태이상은 저지력/자연증가 없이 순수 연타(Passive/Decay=0).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Escape")
	FTStruggleGaugeStruct Gauge;

private:
	// Event.Struggle(발버둥 1회) 수신 → 게이지 상승. 가득 차면 Escapable을 부여한 GE 제거 후 종료.
	UFUNCTION()
	void OnStruggleEvent(FGameplayEventData Payload);
};
