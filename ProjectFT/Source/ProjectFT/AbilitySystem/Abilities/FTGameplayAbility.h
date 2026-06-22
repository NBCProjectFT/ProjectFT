// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "FTGameplayAbility.generated.h"

/**
 * 프로젝트 공용 GameplayAbility 베이스. 인스턴싱/실행 정책 등 공통 기본값을 모은다.
 */
UCLASS(Abstract)
class PROJECTFT_API UFTGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFTGameplayAbility();
};
