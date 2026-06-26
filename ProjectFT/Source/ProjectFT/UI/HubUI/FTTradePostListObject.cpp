#include "FTTradePostListObject.h"

void UFTTradePostListObject::Initialize(const FTTradePostStruct& InTradePost)
{
	TradePost = InTradePost;
}

const FTTradePostStruct& UFTTradePostListObject::GetTradePost() const
{
	return TradePost;
}
