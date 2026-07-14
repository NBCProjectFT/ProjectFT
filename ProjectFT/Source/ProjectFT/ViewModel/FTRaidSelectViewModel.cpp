#include "FTRaidSelectViewModel.h"

#include "ProjectFT/Components/FTInventoryComponent.h"

void UFTRaidSelectViewModel::Initialize(AFTHubRaidEntrance* InRaidEntrance, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventory();

	RaidEntrance = InRaidEntrance;
	PlayerInventory = InPlayerInventory;
	Options = RaidEntrance ? RaidEntrance->GetRaidOptions() : TArray<FFTRaidEntranceOption>();
	SelectedOptionIndex = INDEX_NONE;
	bEntryRequestInProgress = false;

	BindInventory();
	OnChanged.Broadcast();
}

bool UFTRaidSelectViewModel::GetOption(const int32 Index, FFTRaidEntranceOption& OutOption) const
{
	if (!Options.IsValidIndex(Index))
	{
		return false;
	}

	OutOption = Options[Index];
	return true;
}

bool UFTRaidSelectViewModel::CanEnterOption(const int32 Index) const
{
	if (bEntryRequestInProgress || !Options.IsValidIndex(Index))
	{
		return false;
	}

	const FFTRaidEntranceOption& Option = Options[Index];
	return !Option.LevelName.IsNone()
		&& (Option.RequiredItemId.IsNone()
			|| (PlayerInventory && PlayerInventory->GetItemQuantity(Option.RequiredItemId) >= 1));
}

void UFTRaidSelectViewModel::SelectOption(const int32 Index)
{
	if (bEntryRequestInProgress || !Options.IsValidIndex(Index))
	{
		return;
	}

	SelectedOptionIndex = Index;
	OnChanged.Broadcast();
}

bool UFTRaidSelectViewModel::ConfirmSelectedOption()
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	if (!Option || !CanEnterOption(SelectedOptionIndex) || !RaidEntrance)
	{
		return false;
	}

	bEntryRequestInProgress = true;
	OnChanged.Broadcast();

	const bool bStarted = RaidEntrance->TryEnterRaid(Option->LevelName, PlayerInventory);
	if (!bStarted)
	{
		bEntryRequestInProgress = false;
		OnChanged.Broadcast();
	}
	return bStarted;
}

FText UFTRaidSelectViewModel::GetSelectedDisplayName() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	return Option ? Option->DisplayName : FText::GetEmpty();
}

FText UFTRaidSelectViewModel::GetSelectedEntryCostText() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	if (!Option)
	{
		return FText::GetEmpty();
	}
	if (Option->RequiredItemId.IsNone())
	{
		return FText::FromString(TEXT("무료 입장"));
	}

	const int32 OwnedCount = PlayerInventory ? PlayerInventory->GetItemQuantity(Option->RequiredItemId) : 0;
	return FText::FromString(FString::Printf(
		TEXT("%s  %d / 1"),
		*Option->RequiredItemId.ToString(),
		OwnedCount));
}

FText UFTRaidSelectViewModel::GetSelectedStatusText() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	if (!Option)
	{
		return FText::GetEmpty();
	}

	return CanEnterOption(SelectedOptionIndex)
		? (Option->RequiredItemId.IsNone()
			? FText::FromString(TEXT("무료로 입장할 수 있습니다."))
			: FText::FromString(TEXT("입장권 1개를 소모합니다.")))
		: FText::FromString(TEXT("필요한 입장권이 없습니다."));
}

void UFTRaidSelectViewModel::HandleInventoryChanged()
{
	OnChanged.Broadcast();
}

const FFTRaidEntranceOption* UFTRaidSelectViewModel::GetSelectedOption() const
{
	return Options.IsValidIndex(SelectedOptionIndex) ? &Options[SelectedOptionIndex] : nullptr;
}

void UFTRaidSelectViewModel::BindInventory()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTRaidSelectViewModel::HandleInventoryChanged);
		PlayerInventory->OnInventoryChanged.AddDynamic(this, &UFTRaidSelectViewModel::HandleInventoryChanged);
	}
}

void UFTRaidSelectViewModel::UnbindInventory()
{
	if (PlayerInventory)
	{
		PlayerInventory->OnInventoryChanged.RemoveDynamic(this, &UFTRaidSelectViewModel::HandleInventoryChanged);
	}
}
