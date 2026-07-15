#include "FTRaidSelectViewModel.h"

#include "Engine/Texture2D.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/UI/HubUI/FTRaidLevelListObject.h"

void UFTRaidSelectViewModel::Initialize(AFTHubRaidEntrance* InRaidEntrance, UFTInventoryComponent* InPlayerInventory)
{
	UnbindInventory();

	RaidEntrance = InRaidEntrance;
	PlayerInventory = InPlayerInventory;
	Options = RaidEntrance ? RaidEntrance->GetRaidOptions() : TArray<FFTRaidEntranceOption>();
	LevelObjects.Reset();
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const FFTRaidEntranceOption& Option = Options[Index];
		UTexture2D* LevelPreview = Option.PreviewImage.IsNull()
			? nullptr
			: Option.PreviewImage.LoadSynchronous();

		UFTRaidLevelListObject* LevelObject = NewObject<UFTRaidLevelListObject>(this);
		LevelObject->Initialize(Option, Index, LevelPreview);
		LevelObjects.Add(LevelObject);
	}
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

void UFTRaidSelectViewModel::SelectLevelObject(UObject* LevelObject)
{
	const UFTRaidLevelListObject* RaidLevelObject = Cast<UFTRaidLevelListObject>(LevelObject);
	if (!RaidLevelObject)
	{
		return;
	}

	SelectOption(RaidLevelObject->GetOptionIndex());
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

FText UFTRaidSelectViewModel::GetSelectedDescription() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	return Option ? Option->Description : FText::GetEmpty();
}

UTexture2D* UFTRaidSelectViewModel::GetSelectedPreviewImage() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	return Option && !Option->PreviewImage.IsNull()
		? Option->PreviewImage.LoadSynchronous()
		: nullptr;
}

UTexture2D* UFTRaidSelectViewModel::GetSelectedRequiredItemIcon() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	if (!Option || Option->RequiredItemId.IsNone() || !PlayerInventory)
	{
		return nullptr;
	}

	const UFTItemDataAsset* ItemData = PlayerInventory->FindItemData(Option->RequiredItemId);
	return ItemData && !ItemData->ItemData.ItemIcon.IsNull()
		? ItemData->ItemData.ItemIcon.LoadSynchronous()
		: nullptr;
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

	FText RequiredItemName = FText::FromString(TEXT("필요 아이템"));
	if (PlayerInventory)
	{
		if (const UFTItemDataAsset* ItemData = PlayerInventory->FindItemData(Option->RequiredItemId))
		{
			if (!ItemData->ItemData.ItemName.IsEmpty())
			{
				RequiredItemName = ItemData->ItemData.ItemName;
			}
		}
	}

	const int32 OwnedCount = PlayerInventory ? PlayerInventory->GetItemQuantity(Option->RequiredItemId) : 0;
	return FText::FromString(FString::Printf(
		TEXT("%s  %d / 1"),
		*RequiredItemName.ToString(),
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
