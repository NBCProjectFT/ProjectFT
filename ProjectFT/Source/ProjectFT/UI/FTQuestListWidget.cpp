#include "FTQuestListWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"

void UFTQuestListWidget::SetQuestTitle(const FText& NewQuestTitle)
{
	if (Text_QuestTitle)
	{
		Text_QuestTitle->SetText(NewQuestTitle);
	}
}

void UFTQuestListWidget::SetEmptyQuestVisible(bool bVisible)
{
	if (Text_EmptyQuest)
	{
		Text_EmptyQuest->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UFTQuestListWidget::ClearQuestEntries()
{
	if (VB_QuestList)
	{
		VB_QuestList->ClearChildren();
	}
}

void UFTQuestListWidget::AddQuestEntryWidget(UWidget* QuestEntryWidget)
{
	if (VB_QuestList && QuestEntryWidget)
	{
		VB_QuestList->AddChild(QuestEntryWidget);
	}
}
