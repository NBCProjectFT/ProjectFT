#include "FTHubRaidEntrance.h"

#include "FTHubActorUtils.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTGameFlowSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"

AFTHubRaidEntrance::AFTHubRaidEntrance()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionPrompt = FText::FromString(TEXT("마트 선택"));

	FFTRaidEntranceOption FreeMarket;
	FreeMarket.DisplayName = FText::FromString(TEXT("마트"));
	FreeMarket.Description = FText::FromString(TEXT("입장 제한이 없는 기본 레이드 레벨입니다."));
	FreeMarket.LevelName = TEXT("Market");
	RaidOptions.Add(FreeMarket);

	FFTRaidEntranceOption TicketMarket;
	TicketMarket.DisplayName = FText::FromString(TEXT("테스트 마트"));
	TicketMarket.Description = FText::FromString(TEXT("입장권 1개가 필요한 레이드 레벨입니다."));
	TicketMarket.LevelName = TEXT("Market_Test");
	TicketMarket.RequiredItemId = TEXT("ID_Common_Floor2_MemberCard");
	RaidOptions.Add(TicketMarket);
}

bool AFTHubRaidEntrance::Interact_Implementation(AActor* Interactor)
{
	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->ShowRaidSelect(this, FTHubActorUtils::FindPlayerInventory(this, Interactor));
		return true;
	}

	return false;
}

FText AFTHubRaidEntrance::GetInteractionPrompt_Implementation() const
{
	return InteractionPrompt;
}

void AFTHubRaidEntrance::RequestStartRaid(AActor* InstigatorActor)
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = InstigatorActor;
	Payload.TargetActor = this;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_FT_Request_Flow_StartRaid, Payload);
}

bool AFTHubRaidEntrance::TryEnterRaid(const FName LevelName, UFTInventoryComponent* PlayerInventory)
{
	const FFTRaidEntranceOption* Option = RaidOptions.FindByPredicate([LevelName](const FFTRaidEntranceOption& Candidate)
	{
		return Candidate.LevelName == LevelName;
	});
	if (!Option)
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UFTGameFlowSubsystem* FlowSubsystem = GameInstance ? GameInstance->GetSubsystem<UFTGameFlowSubsystem>() : nullptr;
	if (!FlowSubsystem || !FlowSubsystem->CanStartRaidAtLevel(LevelName))
	{
		return false;
	}

	const bool bConsumesItem = !Option->RequiredItemId.IsNone();
	if (bConsumesItem && (!PlayerInventory || PlayerInventory->GetItemQuantity(Option->RequiredItemId) < 1))
	{
		return false;
	}

	if (bConsumesItem && !PlayerInventory->RemoveItem(Option->RequiredItemId, 1))
	{
		return false;
	}

	if (!FlowSubsystem->RequestStartRaidAtLevel(LevelName))
	{
		if (bConsumesItem)
		{
			PlayerInventory->AddItem(Option->RequiredItemId, 1);
		}
		return false;
	}

	if (UFTUIManagerSubsystem* UIManager = FTHubActorUtils::GetUIManager(this))
	{
		UIManager->HideRaidSelect();
	}

	return true;
}
