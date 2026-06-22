// Fill out your copyright notice in the Description page of Project Settings.

#include "FTGameplayAbility.h"

UFTGameplayAbility::UFTGameplayAbility()
{
	// 시전 대기(AbilityTask) 등 인스턴스 상태가 필요하므로 액터당 인스턴스화한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}
