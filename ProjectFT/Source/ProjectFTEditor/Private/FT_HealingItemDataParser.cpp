#include "FT_HealingItemDataParser.h"

#include "Engine/DataTable.h"
#include "ProjectFT/Enum/FTItemCategoryType.h"
#include "ProjectFT/Struct/FTHealingItemTableRowStruct.h"

namespace
{
	bool IsUnsetConfigValue(const FString& Value)
	{
		const FString TrimmedValue = Value.TrimStartAndEnd();
		return TrimmedValue.IsEmpty()
			|| TrimmedValue.Equals(TEXT("None"), ESearchCase::IgnoreCase);
	}
}

void UFT_HealingItemDataParser::OnParseComplete()
{
	UE_LOG(LogTemp, Log, TEXT("[HealingItemSheet] Parsed rows: %d"), GetRowCount());

	if (!IsValid(TargetTable))
	{
		UE_LOG(LogTemp, Error, TEXT("[HealingItemSheet] TargetTable is empty."));
		return;
	}

	if (TargetTable->GetRowStruct() != FTHealingItemTableRowStruct::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("[HealingItemSheet] TargetTable row struct must be FTHealingItemTableRowStruct."));
		return;
	}

	TargetTable->Modify();

	const FString HeaderLog = FString::Join(GetHeaders(), TEXT(", "));
	UE_LOG(LogTemp, Log, TEXT("[HealingItemSheet] Headers: %s"), *HeaderLog);

	for (int32 RowIndex = 0; RowIndex < GetRowCount(); ++RowIndex)
	{
		TMap<FString, FString> RowData;
		if (!GetRowAt(RowIndex, RowData))
		{
			continue;
		}

		const FString ID = RowData.FindRef(TEXT("ID")).TrimStartAndEnd();
		if (IsUnsetConfigValue(ID))
		{
			UE_LOG(LogTemp, Warning, TEXT("[HealingItemSheet] Skip row %d because ID is empty."), RowIndex);
			continue;
		}

		FTHealingItemTableRowStruct NewRow;
		NewRow.ItemData.ItemId = FName(*ID);
		NewRow.ItemData.ItemName = FText::FromString(RowData.FindRef(TEXT("DisplayName")).TrimStartAndEnd());
		NewRow.ItemData.ItemDescription = FText::FromString(RowData.FindRef(TEXT("Description")).TrimStartAndEnd());
		NewRow.ItemData.CategoryType = EFTItemCategoryType::Healing;

		TargetTable->AddRow(NewRow.ItemData.ItemId, NewRow);
	}

	TargetTable->MarkPackageDirty();
}
