#include "FTItemTooltipWidget.h"
#include "Components/TextBlock.h"

void UFTItemTooltipWidget::SetItemInfo(const FText& Name, float Weight, int32 Cost)
{
	if (Text_ItemName)
	{
		Text_ItemName->SetText(Name);
	}

	if (Text_Weight)
	{
		FText WeightText = FText::Format(NSLOCTEXT("ItemUI", "WeightFormat", "{0} kg"), FText::AsNumber(Weight));
		Text_Weight->SetText(WeightText);
	}

	if (Text_Cost)
	{
		FText CostText = FText::Format(NSLOCTEXT("ItemUI", "CostFormat", "{0} G"), FText::AsNumber(Cost));
		Text_Cost->SetText(CostText);
	}
}