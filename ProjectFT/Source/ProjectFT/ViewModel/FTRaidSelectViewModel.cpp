#include "FTRaidSelectViewModel.h"

#include "Engine/Texture2D.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Item/FTItemFunctionLibrary.h"
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
		UFTRaidLevelListObject* LevelObject = NewObject<UFTRaidLevelListObject>(this);
		LevelObject->Initialize(Option, Index);
		LevelObjects.Add(LevelObject);
	}
	SelectedOptionIndex = LevelObjects.IsEmpty() ? INDEX_NONE : 0;
	bEntryRequestInProgress = false;

	BindInventory();
	OnChanged.Broadcast();
}

TArray<UObject*> UFTRaidSelectViewModel::GetLevelObjects() const
{
	TArray<UObject*> Result;
	Result.Reserve(LevelObjects.Num());
	for (UObject* LevelObject : LevelObjects)
	{
		Result.Add(LevelObject);
	}
	return Result;
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

bool UFTRaidSelectViewModel::HasSelectedOption() const
{
	return GetSelectedOption() != nullptr;
}

UFTRaidLevelListObject* UFTRaidSelectViewModel::GetSelectedLevelObject() const
{
	return LevelObjects.IsValidIndex(SelectedOptionIndex)
		? Cast<UFTRaidLevelListObject>(LevelObjects[SelectedOptionIndex])
		: nullptr;
}

bool UFTRaidSelectViewModel::CanEnterSelectedOption() const
{
	return CanEnterOption(SelectedOptionIndex);
}

bool UFTRaidSelectViewModel::IsSelectedOptionFree() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	return Option && Option->RequiredItemId.IsNone();
}

FName UFTRaidSelectViewModel::GetSelectedRequiredItemID() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	return Option ? Option->RequiredItemId : NAME_None;
}

FText UFTRaidSelectViewModel::GetSelectedRequiredItemName() const
{
	const FName RequiredItemID = GetSelectedRequiredItemID();
	if (RequiredItemID.IsNone())
	{
		return FText::GetEmpty();
	}

	const UFTItemDataAsset* ItemData = UFTItemFunctionLibrary::FindItemData(this, RequiredItemID);
	return ItemData && !ItemData->ItemData.ItemName.IsEmpty()
		? ItemData->ItemData.ItemName
		: FText::FromName(RequiredItemID);
}

int32 UFTRaidSelectViewModel::GetSelectedRequiredItemOwnedCount() const
{
	const FName RequiredItemID = GetSelectedRequiredItemID();
	return !RequiredItemID.IsNone() && PlayerInventory
		? PlayerInventory->GetItemQuantity(RequiredItemID)
		: 0;
}

int32 UFTRaidSelectViewModel::GetSelectedRequiredItemCount() const
{
	return IsSelectedOptionFree() ? 0 : 1;
}

TSoftObjectPtr<UTexture2D> UFTRaidSelectViewModel::GetSelectedPreviewImageSoft() const
{
	const FFTRaidEntranceOption* Option = GetSelectedOption();
	return Option ? Option->PreviewImage : TSoftObjectPtr<UTexture2D>();
}

TSoftObjectPtr<UTexture2D> UFTRaidSelectViewModel::GetSelectedRequiredItemIconSoft() const
{
	const FName RequiredItemID = GetSelectedRequiredItemID();
	if (RequiredItemID.IsNone())
	{
		return TSoftObjectPtr<UTexture2D>();
	}

	const UFTItemDataAsset* ItemData = UFTItemFunctionLibrary::FindItemData(this, RequiredItemID);
	return ItemData ? ItemData->ItemData.ItemIcon : TSoftObjectPtr<UTexture2D>();
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
