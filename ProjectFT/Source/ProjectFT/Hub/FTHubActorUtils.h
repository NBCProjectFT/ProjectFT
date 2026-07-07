#pragma once

#include "CoreMinimal.h"

class AActor;
class UFTInventoryComponent;
class UFTUIManagerSubsystem;

namespace FTHubActorUtils
{
	UFTInventoryComponent* FindPlayerInventory(const AActor* ContextActor, AActor* Interactor);
	UFTUIManagerSubsystem* GetUIManager(const AActor* ContextActor);
}
