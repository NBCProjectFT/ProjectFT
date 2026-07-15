#include "FTQuestListWidget.h"

#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Core/FTObjectiveSubsystem.h"
#include "ProjectFT/Enum/FTQuestStateType.h"
#include "ProjectFT/Hub/FTHubStorage.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "ProjectFT/Struct/FTQuestStruct.h"

void UFTQuestListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UnregisterQuestListeners();
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	QuestProgressListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_ObjectiveProgressChanged,
		this,
		&ThisClass::HandleQuestProgressChanged);
	QuestCompletedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_ObjectiveCompleted,
		this,
		&ThisClass::HandleQuestCompleted);
	BindInventoryListeners();

	RefreshQuestList();
}

void UFTQuestListWidget::NativeDestruct()
{
	UnbindInventoryListeners();
	UnregisterQuestListeners();
	Super::NativeDestruct();
}

void UFTQuestListWidget::HandleInventoryChanged()
{
	RefreshQuestList();
}

void UFTQuestListWidget::BindInventoryListeners()
{
	UnbindInventoryListeners();

	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController && GetWorld())
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	if (APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr)
	{
		BoundPlayerInventory = Pawn->FindComponentByClass<UFTInventoryComponent>();
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UFTObjectiveSubsystem* ObjectiveSubsystem = GameInstance->GetSubsystem<UFTObjectiveSubsystem>())
		{
			if (AFTHubStorage* HubStorage = ObjectiveSubsystem->GetHubStorage())
			{
				BoundStorageInventory = HubStorage->GetStorageInventory();
			}
		}
	}

	if (BoundPlayerInventory)
	{
		BoundPlayerInventory->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::HandleInventoryChanged);
	}
	if (BoundStorageInventory && BoundStorageInventory != BoundPlayerInventory)
	{
		BoundStorageInventory->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::HandleInventoryChanged);
	}
}

void UFTQuestListWidget::UnbindInventoryListeners()
{
	if (BoundPlayerInventory)
	{
		BoundPlayerInventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::HandleInventoryChanged);
	}
	if (BoundStorageInventory)
	{
		BoundStorageInventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::HandleInventoryChanged);
	}

	BoundPlayerInventory = nullptr;
	BoundStorageInventory = nullptr;
}

void UFTQuestListWidget::RefreshQuestList()
{
	const UGameInstance* GameInstance = GetGameInstance();
	UFTObjectiveSubsystem* ObjectiveSubsystem = GameInstance
		? GameInstance->GetSubsystem<UFTObjectiveSubsystem>()
		: nullptr;
	if (!ObjectiveSubsystem)
	{
		return;
	}

	TArray<FTQuestStruct> ActiveQuests;
	ObjectiveSubsystem->GetQuestListByState(EFTQuestStateType::Active, ActiveQuests);
	ActiveQuests.Sort([](const FTQuestStruct& Left, const FTQuestStruct& Right)
	{
		return Left.QuestID.LexicalLess(Right.QuestID);
	});

	const TArray<UHorizontalBox*> QuestRows = { HB_QuestRow_0, HB_QuestRow_1, HB_QuestRow_2 };
	const TArray<UCheckBox*> QuestCheckBoxes = { CB_Quest_0, CB_Quest_1, CB_Quest_2 };
	const TArray<UTextBlock*> QuestTexts = { Text_Quest_0, Text_Quest_1, Text_Quest_2 };
	const TArray<UTextBlock*> QuestCountTexts = { Text_QuestCount_0, Text_QuestCount_1, Text_QuestCount_2 };
	const TArray<UTextBlock*> QuestDetailTexts = { Text_QuestDetail_0, Text_QuestDetail_1, Text_QuestDetail_2 };

	for (int32 RowIndex = 0; RowIndex < QuestRows.Num(); ++RowIndex)
	{
		const bool bHasQuest = ActiveQuests.IsValidIndex(RowIndex);
		if (QuestRows[RowIndex])
		{
			QuestRows[RowIndex]->SetVisibility(bHasQuest ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}

		if (!bHasQuest)
		{
			continue;
		}

		const FTQuestStruct& Quest = ActiveQuests[RowIndex];

		if (QuestTexts[RowIndex])
		{
			QuestTexts[RowIndex]->SetText(Quest.QuestName);
		}

		if (QuestCountTexts[RowIndex])
		{
			// 전체 합산 카운트는 표시하지 않는다. 상세 목표별 카운트만 사용한다.
			QuestCountTexts[RowIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (QuestDetailTexts[RowIndex])
		{
			const FText DetailText = ObjectiveSubsystem->GetQuestObjectiveProgressText(Quest.QuestID);
			QuestDetailTexts[RowIndex]->SetText(DetailText);
			QuestDetailTexts[RowIndex]->SetVisibility(
				DetailText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}

		if (QuestCheckBoxes[RowIndex])
		{
			QuestCheckBoxes[RowIndex]->SetIsEnabled(false);
			QuestCheckBoxes[RowIndex]->SetIsChecked(ObjectiveSubsystem->GetQuestProgress(Quest.QuestID) >= 1.0f);
		}
	}

	SetEmptyQuestVisible(ActiveQuests.IsEmpty());
}

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
	const TArray<UHorizontalBox*> QuestRows = { HB_QuestRow_0, HB_QuestRow_1, HB_QuestRow_2 };
	for (UHorizontalBox* QuestRow : QuestRows)
	{
		if (QuestRow)
		{
			QuestRow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UFTQuestListWidget::AddQuestEntryWidget(UWidget* QuestEntryWidget)
{
	if (VB_QuestList && QuestEntryWidget)
	{
		VB_QuestList->AddChild(QuestEntryWidget);
	}
}

void UFTQuestListWidget::HandleQuestProgressChanged(
	FGameplayTag Channel,
	const FFTMessagePayloadStruct& Payload)
{
	RefreshQuestList();
}

void UFTQuestListWidget::HandleQuestCompleted(
	FGameplayTag Channel,
	const FFTMessagePayloadStruct& Payload)
{
	RefreshQuestList();
}

void UFTQuestListWidget::UnregisterQuestListeners()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	if (QuestProgressListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(QuestProgressListenerHandle);
		QuestProgressListenerHandle = FGameplayMessageListenerHandle();
	}

	if (QuestCompletedListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(QuestCompletedListenerHandle);
		QuestCompletedListenerHandle = FGameplayMessageListenerHandle();
	}
}
