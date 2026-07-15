#include "FTRaidLevelEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "FTRaidLevelListObject.h"

void UFTRaidLevelEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UFTRaidLevelListObject* LevelObject = Cast<UFTRaidLevelListObject>(ListItemObject);
	if (!LevelObject)
	{
		return;
	}

	const FFTRaidEntranceOption& Option = LevelObject->GetOption();
	TXT_LevelName->SetText(Option.DisplayName);

	if (UTexture2D* LevelPreview = LevelObject->GetLevelPreview())
	{
		IMG_LevelPreview->SetBrushFromTexture(LevelPreview);
		IMG_LevelPreview->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		IMG_LevelPreview->SetVisibility(ESlateVisibility::Collapsed);
	}

	ApplySelectionVisual(IsListItemSelected());
}

void UFTRaidLevelEntryWidget::NativeOnItemSelectionChanged(const bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
	ApplySelectionVisual(bIsSelected);
}

void UFTRaidLevelEntryWidget::ApplySelectionVisual(const bool bIsSelected)
{
	const float Opacity = bIsSelected ? 1.0f : 0.45f;
	TXT_LevelName->SetRenderOpacity(Opacity);
	IMG_LevelPreview->SetRenderOpacity(Opacity);
}
