// Fill out your copyright notice in the Description page of Project Settings.


#include "FTGameMode.h"

#include "FTGameFlowSubsystem.h"

void AFTGameMode::StartPlay()
{
	Super::StartPlay();
}

void AFTGameMode::HandleRaidStart()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			GameFlowSubsystem->RequestStartRaid();
		}
	}
}

void AFTGameMode::HandleRaidFail()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			GameFlowSubsystem->RequestFailRaid();
		}
	}
}

void AFTGameMode::HandleRaidEscape()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFTGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UFTGameFlowSubsystem>())
		{
			GameFlowSubsystem->RequestEscapeRaid();
		}
	}
}
