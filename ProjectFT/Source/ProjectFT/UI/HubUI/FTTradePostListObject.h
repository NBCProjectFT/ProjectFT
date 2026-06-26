#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTTradePostStruct.h"
#include "FTTradePostListObject.generated.h"

UCLASS()
class PROJECTFT_API UFTTradePostListObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FTTradePostStruct& InTradePost);
	const FTTradePostStruct& GetTradePost() const;

private:
	FTTradePostStruct TradePost;
};
