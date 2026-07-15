#pragma once

#include "CoreMinimal.h"
#include "GoogleSheetParserBase.h"
#include "FT_QuestDataParser.generated.h"

class UDataTable;

/** Google Sheet의 Quest 페이지를 FTQuestStruct 기반 DataTable로 변환한다. */
UCLASS(BlueprintType, EditInlineNew)
class PROJECTFTEDITOR_API UFT_QuestDataParser : public UGoogleSheetParserBase
{
	GENERATED_BODY()

public:
	/** 공통 CSV/GViz 파싱이 끝난 뒤 모든 행을 검증하고 TargetTable을 갱신한다. */
	virtual void OnParseComplete() override;

protected:
	/** 변환 결과를 저장할 DT_QuestList 계열 DataTable. Config 에셋에서 지정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UDataTable> TargetTable = nullptr;
};
