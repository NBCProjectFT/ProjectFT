#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FTQuestConditionStruct.generated.h"

/**
 * GameplayMessage 한 종류를 몇 번 받아야 하는지 정의하는 퀘스트 조건.
 * ItemID가 비어 있지 않으면 해당 아이템 메시지만 조건에 반영한다.
 */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTQuestConditionStruct
{
	GENERATED_BODY()

	/** 조건 진행에 사용할 GameplayMessage 채널. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Quest")
	FGameplayTag EventTag;

	/** 완료에 필요한 이벤트 발생 횟수. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Quest", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;

	/** 선택적 아이템 필터. NAME_None이면 ItemID를 검사하지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Quest")
	FName ItemID = NAME_None;

	bool IsValid() const
	{
		return EventTag.IsValid() && RequiredCount > 0;
	}
};
