#include "FTHubCraftWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Engine/Texture2D.h"
#include "ProjectFT/Core/FTCraftingSubsystem.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTCraftingViewModel.h"

void UFTHubCraftWidget::InitializeCraftWidget(
	UFTInventoryComponent* InPlayerInventory,
	UFTInventoryComponent* InStorageInventory,
	UFTCraftingViewModel* InViewModel)
{
	if (ViewModel != InViewModel)
	{
		if (ViewModel)
		{
			ViewModel->OnChanged.RemoveDynamic(this, &UFTHubCraftWidget::RefreshFromViewModel);
		}

		ViewModel = InViewModel ? InViewModel : NewObject<UFTCraftingViewModel>(this);
		if (ViewModel)
		{
			ViewModel->OnChanged.RemoveDynamic(this, &UFTHubCraftWidget::RefreshFromViewModel);
			ViewModel->OnChanged.AddDynamic(this, &UFTHubCraftWidget::RefreshFromViewModel);
		}
	}

	if (ViewModel)
	{
		UFTCraftingSubsystem* CraftingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UFTCraftingSubsystem>()
			: nullptr;
		ViewModel->Initialize(CraftingSubsystem, InPlayerInventory, InStorageInventory);
	}

	RefreshFromViewModel();
}

void UFTHubCraftWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LV_CraftRecipes)
	{
		LV_CraftRecipes->OnItemClicked().RemoveAll(this);
		LV_CraftRecipes->OnItemClicked().AddUObject(this, &UFTHubCraftWidget::HandleRecipeClicked);
	}

	if (UCheckBox* CraftableOnlyCheckBox = GetCraftableOnlyCheckBox())
	{
		CraftableOnlyCheckBox->OnCheckStateChanged.RemoveDynamic(this, &UFTHubCraftWidget::HandleCraftableOnlyChanged);
		CraftableOnlyCheckBox->OnCheckStateChanged.AddDynamic(this, &UFTHubCraftWidget::HandleCraftableOnlyChanged);
	}

	if (EDT_SearchRecipe)
	{
		EDT_SearchRecipe->OnTextChanged.RemoveDynamic(this, &UFTHubCraftWidget::HandleSearchRecipeTextChanged);
		EDT_SearchRecipe->OnTextChanged.AddDynamic(this, &UFTHubCraftWidget::HandleSearchRecipeTextChanged);
	}

	if (BTN_Craft)
	{
		BTN_Craft->OnClicked.RemoveDynamic(this, &UFTHubCraftWidget::HandleCraftClicked);
		BTN_Craft->OnClicked.AddDynamic(this, &UFTHubCraftWidget::HandleCraftClicked);
		BTN_Craft->SetIsEnabled(false);
	}

	if (BTN_Close)
	{
		BTN_Close->OnClicked.RemoveDynamic(this, &UFTHubCraftWidget::HandleCloseClicked);
		BTN_Close->OnClicked.AddDynamic(this, &UFTHubCraftWidget::HandleCloseClicked);
	}

	RefreshFromViewModel();
}

void UFTHubCraftWidget::RefreshFromViewModel()
{
	if (!ViewModel)
	{
		return;
	}

	PopulateItems(GetStorageItemsView(), ViewModel->GetStorageItemObjects());
	PopulateItems(LV_CraftRecipes, ViewModel->GetRecipeObjects());
	PopulateItems(TV_RequiredItems, ViewModel->GetRequiredItemObjects());

	if (TXT_RecipeCount)
	{
		TXT_RecipeCount->SetText(ViewModel->GetRecipeCountText());
	}

	if (TXT_SelectedRecipeName)
	{
		TXT_SelectedRecipeName->SetText(ViewModel->GetSelectedRecipeNameText());
	}

	if (TXT_RequiredItems)
	{
		TXT_RequiredItems->SetText(ViewModel->GetRequiredItemsText());
	}

	if (TXT_ResultItem)
	{
		TXT_ResultItem->SetText(ViewModel->GetResultItemText());
	}

	if (TXT_SelectedRecipeTier)
	{
		TXT_SelectedRecipeTier->SetText(ViewModel->GetSelectedRecipeTierText());
	}

	if (TXT_SelectedRecipeDescription)
	{
		TXT_SelectedRecipeDescription->SetText(ViewModel->GetSelectedRecipeDescriptionText());
	}

	if (TXT_CraftTime)
	{
		TXT_CraftTime->SetText(ViewModel->GetCraftTimeText());
	}

	if (TXT_CraftAmount)
	{
		TXT_CraftAmount->SetText(ViewModel->GetCraftAmountText());
	}

	if (IMG_ResultItemIcon)
	{
		if (UTexture2D* IconTexture = ViewModel->GetResultItemIcon())
		{
			IMG_ResultItemIcon->SetBrushFromTexture(IconTexture);
		}
	}

	if (BTN_Craft)
	{
		BTN_Craft->SetIsEnabled(ViewModel->CanCraftSelectedRecipe());
	}
}

UListView* UFTHubCraftWidget::GetStorageItemsView() const
{
	return Cast<UListView>(TV_StorageItems);
}

UCheckBox* UFTHubCraftWidget::GetCraftableOnlyCheckBox() const
{
	return CHK_ShowCraftableOnly ? CHK_ShowCraftableOnly : CHK_CraftableOnly;
}

void UFTHubCraftWidget::PopulateItems(UListView* ItemsView, const TArray<TObjectPtr<UObject>>& Items)
{
	if (!ItemsView)
	{
		return;
	}

	ItemsView->ClearListItems();
	for (UObject* Item : Items)
	{
		ItemsView->AddItem(Item);
	}
}

void UFTHubCraftWidget::HandleRecipeClicked(UObject* Item)
{
	if (ViewModel)
	{
		ViewModel->SelectRecipeObject(Item);
	}
}

void UFTHubCraftWidget::HandleCraftableOnlyChanged(const bool bIsChecked)
{
	if (ViewModel)
	{
		ViewModel->SetCraftableOnly(bIsChecked);
	}
}

void UFTHubCraftWidget::HandleSearchRecipeTextChanged(const FText& Text)
{
	if (ViewModel)
	{
		ViewModel->SetSearchText(Text);
	}
}

void UFTHubCraftWidget::HandleCraftClicked()
{
	if (ViewModel)
	{
		ViewModel->CraftSelectedRecipe();
	}
}

void UFTHubCraftWidget::HandleCloseClicked()
{
	if (UFTUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFTUIManagerSubsystem>()
		: nullptr)
	{
		UIManager->HideCrafting();
	}
}
