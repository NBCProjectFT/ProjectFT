// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectFT.h"
#include "Modules/ModuleManager.h"
#include "AbilitySystemGlobals.h"

// GAS를 쓰려면 전역 데이터 초기화가 한 번 필요하다(TargetData/GameplayCue 등). 게임 모듈 시작 시 호출한다.
// class FProjectFTModule : public FDefaultGameModuleImpl
// {
// public:
// 	virtual void StartupModule() override
// 	{
// 		FDefaultGameModuleImpl::StartupModule();
// 		UAbilitySystemGlobals::Get().InitGlobalData();
// 	}
// };
//
// IMPLEMENT_PRIMARY_GAME_MODULE( FProjectFTModule, ProjectFT, "ProjectFT" );

IMPLEMENT_PRIMARY_GAME_MODULE(
	FDefaultGameModuleImpl,
	ProjectFT,
	"ProjectFT"
);