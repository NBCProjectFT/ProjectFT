#include "FT_CraftRecipeDataParser.h"

#include "Engine/DataTable.h"
#include "ProjectFT/Struct/FTCraftIngredientStruct.h"
#include "ProjectFT/Struct/FTCraftRecipeStruct.h"

namespace
{
	// 헤더가 없을 때도 빈 문자열을 안전하게 반환하고 셀 앞뒤 공백을 제거한다.
	FString GetTrimmedValue(const TMap<FString, FString>& RowData, const TCHAR* Header)
	{
		return RowData.FindRef(Header).TrimStartAndEnd();
	}

	bool ParsePositiveInteger(const FString& Source, int32& OutValue)
	{
		// 재료 수량과 결과 수량은 1 이상의 정수만 허용한다.
		const FString TrimmedSource = Source.TrimStartAndEnd();
		if (!TrimmedSource.IsNumeric())
		{
			return false;
		}

		OutValue = FCString::Atoi(*TrimmedSource);
		return OutValue > 0;
	}

	bool ParseIngredientList(
		const FString& Source,
		TArray<FTCraftIngredientStruct>& OutValues,
		FString& OutError)
	{
		// 입력 형식: ItemID:Count|ItemID:Count
		// 예: ID_Healing_Water:1|ID_Common_Soap:1
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
			// 마지막 ':'를 기준으로 아이템 ID와 수량을 분리한다.
			if (!Token.Split(
				TEXT(":"),
				&ItemIDString,
				&CountString,
				ESearchCase::CaseSensitive,
				ESearchDir::FromEnd))
			{
				OutError = FString::Printf(TEXT("Ingredient must use ItemID:Count format: %s"), *Token);
				return false;
			}

			ItemIDString.TrimStartAndEndInline();
			int32 Count = 0;
			if (ItemIDString.IsEmpty() || !ParsePositiveInteger(CountString, Count))
			{
				OutError = FString::Printf(TEXT("Invalid ingredient: %s"), *Token);
				return false;
			}

			FTCraftIngredientStruct Ingredient;
			Ingredient.ItemID = FName(*ItemIDString);
			Ingredient.Count = Count;
			OutValues.Add(Ingredient);
		}

		// 제작 레시피는 최소 하나 이상의 재료가 필요하다.
		if (OutValues.IsEmpty())
		{
			OutError = TEXT("RequiredItems must contain at least one ItemID:Count entry.");
			return false;
		}

		return true;
	}
}

void UFT_CraftRecipeDataParser::OnParseComplete()
{
	UE_LOG(LogTemp, Log, TEXT("[CraftRecipeSheet] Parsed rows: %d"), GetRowCount());

	// Config에서 지정한 대상과 RowStruct가 올바른지 먼저 확인한다.
	if (!IsValid(TargetTable))
	{
		UE_LOG(LogTemp, Error, TEXT("[CraftRecipeSheet] TargetTable is empty."));
		return;
	}

	if (TargetTable->GetRowStruct() != FTCraftRecipeStruct::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("[CraftRecipeSheet] TargetTable row struct must be FTCraftRecipeStruct."));
		return;
	}

	// 헤더 이름은 Google Sheet 첫 행과 대소문자까지 정확히 일치해야 한다.
	const TArray<FString> RequiredHeaders = {
		TEXT("RecipeID"), TEXT("RequiredItems"), TEXT("ResultItemID"), TEXT("ResultCount")
	};
	const TArray<FString> ActualHeaders = GetHeaders();
	for (const FString& Header : RequiredHeaders)
	{
		if (!ActualHeaders.Contains(Header))
		{
			UE_LOG(LogTemp, Error, TEXT("[CraftRecipeSheet] Missing required header: %s"), *Header);
			return;
		}
	}

	// 검증 도중 기존 DataTable이 반쯤 변경되지 않도록 임시 Map에 먼저 파싱한다.
	// RecipeID는 구조체 ID이면서 최종 DataTable의 RowName으로 사용한다.
	TMap<FName, FTCraftRecipeStruct> ParsedRecipes;
	bool bHasError = false;
	for (int32 RowIndex = 0; RowIndex < GetRowCount(); ++RowIndex)
	{
		TMap<FString, FString> RowData;
		if (!GetRowAt(RowIndex, RowData))
		{
			continue;
		}

		const FString RecipeIDString = GetTrimmedValue(RowData, TEXT("RecipeID"));
		// 조회 범위를 넉넉하게 잡아도 사용할 수 있도록 완전히 빈 행은 건너뛴다.
		if (RecipeIDString.IsEmpty())
		{
			continue;
		}

		const FName RecipeID(*RecipeIDString);
		if (ParsedRecipes.Contains(RecipeID))
		{
			UE_LOG(LogTemp, Error, TEXT("[CraftRecipeSheet] Duplicate RecipeID at row %d: %s"), RowIndex + 2, *RecipeIDString);
			bHasError = true;
			continue;
		}

		FTCraftRecipeStruct NewRow;
		NewRow.RecipeID = RecipeID;

		// 재료 문자열 전체가 유효한 경우에만 구조체에 저장한다.
		FString ParseError;
		if (!ParseIngredientList(GetTrimmedValue(RowData, TEXT("RequiredItems")), NewRow.RequiredItems, ParseError))
		{
			UE_LOG(LogTemp, Error, TEXT("[CraftRecipeSheet] Row %d (%s): %s"), RowIndex + 2, *RecipeIDString, *ParseError);
			bHasError = true;
			continue;
		}

		const FString ResultItemIDString = GetTrimmedValue(RowData, TEXT("ResultItemID"));
		if (ResultItemIDString.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("[CraftRecipeSheet] ResultItemID is empty at row %d (%s)."), RowIndex + 2, *RecipeIDString);
			bHasError = true;
			continue;
		}
		NewRow.ResultItemID = FName(*ResultItemIDString);

		const FString ResultCountString = GetTrimmedValue(RowData, TEXT("ResultCount"));
		if (!ParsePositiveInteger(ResultCountString, NewRow.ResultCount))
		{
			UE_LOG(LogTemp, Error, TEXT("[CraftRecipeSheet] Invalid ResultCount at row %d (%s): %s"),
				RowIndex + 2, *RecipeIDString, *ResultCountString);
			bHasError = true;
			continue;
		}

		ParsedRecipes.Add(RecipeID, MoveTemp(NewRow));
	}

	// 한 행이라도 잘못되면 기존 DataTable을 그대로 유지한다.
	if (bHasError || ParsedRecipes.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[CraftRecipeSheet] Import aborted. The existing DataTable was not changed."));
		return;
	}

	// 전체 검증이 끝난 경우에만 시트 데이터를 단일 원본으로 삼아 테이블을 교체한다.
	TargetTable->Modify();
	TargetTable->EmptyTable();
	for (const TPair<FName, FTCraftRecipeStruct>& Pair : ParsedRecipes)
	{
		TargetTable->AddRow(Pair.Key, Pair.Value);
	}
	TargetTable->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[CraftRecipeSheet] Imported %d recipes into %s."),
		ParsedRecipes.Num(), *TargetTable->GetName());
}
