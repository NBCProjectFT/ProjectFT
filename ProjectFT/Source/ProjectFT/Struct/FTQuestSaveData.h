#pragma once

#include "CoreMinimal.h"
#include "FTQuestSaveData.generated.h"

/** 한 퀘스트의 GameplayMessage 기반 조건 진행도. */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTQuestEventProgressSaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "FT|Quest|Save")
	FName QuestID = NAME_None;

	/** 인덱스는 FTQuestStruct::EventConditions 배열 인덱스와 대응한다. */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "FT|Quest|Save")
	TArray<int32> EventConditionProgress;
};

/** 세이브 시스템이 저장하고 복원할 수 있는 퀘스트 상태 스냅샷. */
USTRUCT(BlueprintType)
struct PROJECTFT_API FFTQuestSaveData
{
	GENERATED_BODY()

	/** 구버전 세이브의 기본 빈 구조와 실제로 저장된 빈 퀘스트 상태를 구분한다. */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "FT|Quest|Save")
	bool bHasQuestData = false;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "FT|Quest|Save")
	TArray<FName> AvailableQuestIDs;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "FT|Quest|Save")
	TArray<FName> ActiveQuestIDs;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "FT|Quest|Save")
	TArray<FName> CompletedQuestIDs;

	/** UPROPERTY는 중첩 컨테이너 TMap<FName, TArray<int32>>를 지원하지 않으므로 행 배열로 저장한다. */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "FT|Quest|Save")
	TArray<FFTQuestEventProgressSaveData> EventProgressByQuest;

	/** 기존 단일 현재 퀘스트 API와의 호환을 위한 값. 다중 퀘스트 상태의 기준은 ActiveQuestIDs다. */
	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "FT|Quest|Save")
	FName CurrentQuestID = NAME_None;
};
