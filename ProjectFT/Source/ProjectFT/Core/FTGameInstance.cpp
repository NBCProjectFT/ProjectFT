#include "FTGameInstance.h"

#include "FTGameFlowSubsystem.h"

void UFTGameInstance::StartRaid()
{
	if (UFTGameFlowSubsystem* GameFlowSubsystem = GetSubsystem<UFTGameFlowSubsystem>())
	{
		GameFlowSubsystem->RequestStartRaid();
	}
}

void UFTGameInstance::ReturnToBase()
{
	if (UFTGameFlowSubsystem* GameFlowSubsystem = GetSubsystem<UFTGameFlowSubsystem>())
	{
		GameFlowSubsystem->ReturnToBase();
	}
}

void UFTGameInstance::RequestSave()
{
}

void UFTGameInstance::RequestLoad()
{
}
