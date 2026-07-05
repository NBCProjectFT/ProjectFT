#include "FTHubTerminal.h"

#include "FTHubActorUtils.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/Core/FTShopSubsystem.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

AFTHubTerminal::AFTHubTerminal()
	: QuestDataTable(nullptr)
	, HubStorage(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFTHubTerminal::BeginPlay()
{
	Super::BeginPlay();
	ConfigureObjectiveSubsystem();
}

bool AFTHubTerminal::Interact_Implementation(AActor* Interactor)
{
	OpenHubWidget(Interactor);
	return true;
}

FText AFTHubTerminal::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("거점 메뉴 보기"));
}

void AFTHubTerminal::CloseHubWidget()
{
	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->HideHubMain();
	}
}

void AFTHubTerminal::OpenHubWidget(AActor* Interactor)
{
	ConfigureObjectiveSubsystem();

	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->ShowHubMain(this, HubStorage, FTHubActorUtils::FindPlayerInventory(this, Interactor));
	}
}

void AFTHubTerminal::ConfigureObjectiveSubsystem()
{
	UGameInstance* GameInstance = GetGameInstance();
	UFTObjectiveSubsystem* ObjectiveSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTObjectiveSubsystem>() : nullptr;
	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->ConfigureHubQuests(QuestDataTable, HubStorage, InitialQuestIDs);
	}

	UFTShopSubsystem* ShopSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTShopSubsystem>() : nullptr;
	if (ShopSubsystem)
	{
		ShopSubsystem->ConfigureHubStorage(HubStorage);
	}
}
