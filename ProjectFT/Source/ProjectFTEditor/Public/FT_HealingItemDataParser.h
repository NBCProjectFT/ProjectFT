#pragma once

#include "CoreMinimal.h"
#include "GoogleSheetParserBase.h"
#include "FT_HealingItemDataParser.generated.h"

class UDataTable;

UCLASS(BlueprintType, EditInlineNew)
class PROJECTFTEDITOR_API UFT_HealingItemDataParser : public UGoogleSheetParserBase
{
	GENERATED_BODY()

public:
	virtual void OnParseComplete() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UDataTable> TargetTable = nullptr;
};
