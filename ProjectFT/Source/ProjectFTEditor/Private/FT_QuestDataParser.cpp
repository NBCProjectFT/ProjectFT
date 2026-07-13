#include "FT_QuestDataParser.h"

#include "Engine/DataTable.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTQuestConditionStruct.h"
#include "ProjectFT/Struct/FTQuestStruct.h"

namespace
{
	// 아이템 입력 형식: ItemID:Count
	// 여러 값은 파이프 문자로 구분한다. 예: ID_Healing_Water:1|ID_Common_Soap:2
	constexpr TCHAR CountDelimiter = TEXT(':');

	// 헤더가 없을 때도 빈 문자열을 안전하게 반환하고 셀 앞뒤 공백을 제거한다.
	FString GetTrimmedValue(const TMap<FString, FString>& RowData, const TCHAR* Header)
	{
		return RowData.FindRef(Header).TrimStartAndEnd();
	}

	void ParseTextList(const FString& Source, TArray<FText>& OutValues)
	{
		// ObjectiveLines처럼 화면에 표시할 문자열 배열을 "값1|값2" 형식에서 변환한다.
		OutValues.Reset();
		TArray<FString> Tokens;
		Source.ParseIntoArray(Tokens, TEXT("|"), true);
		for (FString& Token : Tokens)
		{
			Token.TrimStartAndEndInline();
			if (!Token.IsEmpty())
			{
				OutValues.Add(FText::FromString(Token));
			}
		}
	}

	void ParseNameList(const FString& Source, TArray<FName>& OutValues)
	{
		// NextQuestIDs, UnlockedShopItemIDs처럼 ID로 사용하는 배열을 변환한다.
		OutValues.Reset();
		TArray<FString> Tokens;
		Source.ParseIntoArray(Tokens, TEXT("|"), true);
		for (FString& Token : Tokens)
		{
			Token.TrimStartAndEndInline();
			if (!Token.IsEmpty())
			{
				OutValues.Add(FName(*Token));
			}
		}
	}

	bool ParseIngredientList(
		const FString& Source,
		TArray<FTCraftIngredientStruct>& OutValues,
		FString& OutError)
	{
		// 빈 셀은 빈 배열로 허용한다. RequiredItems와 RewardItems 모두 이 함수를 공유한다.
		OutValues.Reset();
		TArray<FString> Tokens;
		Source.ParseIntoArray(Tokens, TEXT("|"), true);

		for (FString& Token : Tokens)
		{
			Token.TrimStartAndEndInline();
			if (Token.IsEmpty())
			{
				continue;
			}

			FString ItemIDString;
			FString CountString;
			// ItemID 자체에 밑줄이 포함되므로 마지막 ':'를 기준으로 ID와 수량을 나눈다.
			if (!Token.Split(
				FString::Chr(CountDelimiter),
				&ItemIDString,
				&CountString,
				ESearchCase::CaseSensitive,
				ESearchDir::FromEnd))
			{
				OutError = FString::Printf(TEXT("Ingredient must use ItemID:Count format: %s"), *Token);
				return false;
			}

			ItemIDString.TrimStartAndEndInline();
			CountString.TrimStartAndEndInline();
			const int32 Count = FCString::Atoi(*CountString);
			if (ItemIDString.IsEmpty() || Count <= 0 || !CountString.IsNumeric())
			{
				OutError = FString::Printf(TEXT("Invalid ingredient: %s"), *Token);
				return false;
			}

			FTCraftIngredientStruct Ingredient;
			Ingredient.ItemID = FName(*ItemIDString);
			Ingredient.Count = Count;
			OutValues.Add(Ingredient);
		}

		return true;
	}

	bool ParseEventConditionList(
		const FString& Source,
		TArray<FFTQuestConditionStruct>& OutValues,
		FString& OutError)
	{
		// 형식: EventTag@RequiredCount 또는 EventTag@RequiredCount@ItemID
		// 예: Event.Steal.Completed@3|Event.Item.PickedUp@2@ID_Healing_Water
		OutValues.Reset();
		TArray<FString> Tokens;
		Source.ParseIntoArray(Tokens, TEXT("|"), true);

		for (FString& Token : Tokens)
		{
			Token.TrimStartAndEndInline();
			if (Token.IsEmpty())
			{
				continue;
			}

			TArray<FString> Parts;
			Token.ParseIntoArray(Parts, TEXT("@"), false);
			if (Parts.Num() < 2 || Parts.Num() > 3)
			{
				OutError = FString::Printf(
					TEXT("Event condition must use EventTag@RequiredCount[@ItemID] format: %s"),
					*Token);
				return false;
			}

			for (FString& Part : Parts)
			{
				Part.TrimStartAndEndInline();
			}

			const FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName(*Parts[0]), false);
			const int32 RequiredCount = FCString::Atoi(*Parts[1]);
			if (!EventTag.IsValid() || !Parts[1].IsNumeric() || RequiredCount <= 0)
			{
				OutError = FString::Printf(TEXT("Invalid event condition: %s"), *Token);
				return false;
			}

			FFTQuestConditionStruct Condition;
			Condition.EventTag = EventTag;
			Condition.RequiredCount = RequiredCount;
			if (Parts.Num() == 3 && !Parts[2].IsEmpty())
			{
				Condition.ItemID = FName(*Parts[2]);
			}
			OutValues.Add(MoveTemp(Condition));
		}

		return true;
	}
}

void UFT_QuestDataParser::OnParseComplete()
{
	UE_LOG(LogTemp, Log, TEXT("[QuestSheet] Parsed rows: %d"), GetRowCount());

	// Config에서 지정한 대상과 RowStruct가 올바른지 먼저 확인한다.
	if (!IsValid(TargetTable))
	{
		UE_LOG(LogTemp, Error, TEXT("[QuestSheet] TargetTable is empty."));
		return;
	}

	if (TargetTable->GetRowStruct() != FTQuestStruct::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("[QuestSheet] TargetTable row struct must be FTQuestStruct."));
		return;
	}

	// 헤더 이름은 Google Sheet 첫 행과 대소문자까지 정확히 일치해야 한다.
	const TArray<FString> RequiredHeaders = {
		TEXT("QuestID"), TEXT("QuestName"), TEXT("SenderName"), TEXT("Description"),
		TEXT("ObjectiveLines"), TEXT("RequiredItems"), TEXT("RewardItems"),
		TEXT("CurrencyReward"), TEXT("UnlockedShopItemIDs"), TEXT("NextQuestIDs")
	};
	const TArray<FString> ActualHeaders = GetHeaders();
	for (const FString& Header : RequiredHeaders)
	{
		if (!ActualHeaders.Contains(Header))
		{
			UE_LOG(LogTemp, Error, TEXT("[QuestSheet] Missing required header: %s"), *Header);
			return;
		}
	}

	// 검증 도중 기존 DataTable이 반쯤 변경되지 않도록 임시 Map에 먼저 파싱한다.
	// QuestID는 구조체 ID이면서 최종 DataTable의 RowName으로 사용한다.
	TMap<FName, FTQuestStruct> ParsedQuests;
	bool bHasError = false;
	for (int32 RowIndex = 0; RowIndex < GetRowCount(); ++RowIndex)
	{
		TMap<FString, FString> RowData;
		if (!GetRowAt(RowIndex, RowData))
		{
			continue;
		}

		const FString QuestIDString = GetTrimmedValue(RowData, TEXT("QuestID"));
		// 조회 범위를 넉넉하게 잡아도 사용할 수 있도록 완전히 빈 행은 건너뛴다.
		if (QuestIDString.IsEmpty())
		{
			continue;
		}

		const FName QuestID(*QuestIDString);
		if (ParsedQuests.Contains(QuestID))
		{
			UE_LOG(LogTemp, Error, TEXT("[QuestSheet] Duplicate QuestID at row %d: %s"), RowIndex + 2, *QuestIDString);
			bHasError = true;
			continue;
		}

		FTQuestStruct NewRow;
		NewRow.QuestID = QuestID;
		NewRow.QuestName = FText::FromString(GetTrimmedValue(RowData, TEXT("QuestName")));
		NewRow.SenderName = FText::FromString(GetTrimmedValue(RowData, TEXT("SenderName")));
		NewRow.Description = FText::FromString(GetTrimmedValue(RowData, TEXT("Description")));
		ParseTextList(GetTrimmedValue(RowData, TEXT("ObjectiveLines")), NewRow.ObjectiveLines);

		// EventConditions는 기존 퀘스트 시트와의 호환을 위해 선택 열로 취급한다.
		FString ParseError;
		if (ActualHeaders.Contains(TEXT("EventConditions"))
			&& !ParseEventConditionList(
				GetTrimmedValue(RowData, TEXT("EventConditions")),
				NewRow.EventConditions,
				ParseError))
		{
			UE_LOG(LogTemp, Error, TEXT("[QuestSheet] Row %d (%s): %s"), RowIndex + 2, *QuestIDString, *ParseError);
			bHasError = true;
			continue;
		}

		// RequiredItems/RewardItems 입력 예: ID_Healing_Water:1|ID_Common_Soap:1
		if (!ParseIngredientList(GetTrimmedValue(RowData, TEXT("RequiredItems")), NewRow.RequiredItems, ParseError)
			|| !ParseIngredientList(GetTrimmedValue(RowData, TEXT("RewardItems")), NewRow.RewardItems, ParseError))
		{
			UE_LOG(LogTemp, Error, TEXT("[QuestSheet] Row %d (%s): %s"), RowIndex + 2, *QuestIDString, *ParseError);
			bHasError = true;
			continue;
		}

		// 빈 보상 셀은 0으로 처리하고, 값이 있다면 음수가 아닌 정수만 허용한다.
		const FString CurrencyRewardString = GetTrimmedValue(RowData, TEXT("CurrencyReward"));
		NewRow.CurrencyReward = CurrencyRewardString.IsEmpty() ? 0 : FCString::Atoi(*CurrencyRewardString);
		if (NewRow.CurrencyReward < 0 || (!CurrencyRewardString.IsEmpty() && !CurrencyRewardString.IsNumeric()))
		{
			UE_LOG(LogTemp, Error, TEXT("[QuestSheet] Invalid CurrencyReward at row %d: %s"), RowIndex + 2, *CurrencyRewardString);
			bHasError = true;
			continue;
		}

		ParseNameList(GetTrimmedValue(RowData, TEXT("UnlockedShopItemIDs")), NewRow.UnlockedShopItemIDs);
		ParseNameList(GetTrimmedValue(RowData, TEXT("NextQuestIDs")), NewRow.NextQuestIDs);
		ParsedQuests.Add(QuestID, MoveTemp(NewRow));
	}

	// 모든 행을 읽은 뒤 연결 대상까지 검사한다. 오타로 끊긴 퀘스트 체인을 방지한다.
	for (const TPair<FName, FTQuestStruct>& Pair : ParsedQuests)
	{
		for (const FName& NextQuestID : Pair.Value.NextQuestIDs)
		{
			if (!ParsedQuests.Contains(NextQuestID))
			{
				UE_LOG(LogTemp, Error, TEXT("[QuestSheet] Quest %s references missing NextQuestID: %s"),
					*Pair.Key.ToString(), *NextQuestID.ToString());
				bHasError = true;
			}
		}
	}

	// 한 행이라도 잘못되면 기존 DataTable을 그대로 유지한다.
	if (bHasError || ParsedQuests.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[QuestSheet] Import aborted. The existing DataTable was not changed."));
		return;
	}

	// 전체 검증이 끝난 경우에만 시트 데이터를 단일 원본으로 삼아 테이블을 교체한다.
	TargetTable->Modify();
	TargetTable->EmptyTable();
	for (const TPair<FName, FTQuestStruct>& Pair : ParsedQuests)
	{
		TargetTable->AddRow(Pair.Key, Pair.Value);
	}
	TargetTable->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[QuestSheet] Imported %d quests into %s."), ParsedQuests.Num(), *TargetTable->GetName());
}
