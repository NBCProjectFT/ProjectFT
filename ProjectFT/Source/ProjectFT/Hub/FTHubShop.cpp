#include "FTHubShop.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/UI/HubUI/FTHubShopWidget.h"
#include "ProjectFT/Components/FTInventoryComponent.h"

AFTHubShop::AFTHubShop()
{
	PrimaryActorTick.bCanEverTick = false;

	FTShopItemStruct Floor2Membership;
	Floor2Membership.ItemID = TEXT("2층회원권");
	Floor2Membership.Count = 1;
	Floor2Membership.Price = 0;
	Floor2Membership.bUnlockedByDefault = false;
	Floor2Membership.bFixedSlot = true;
	FixedShopItems.Add(Floor2Membership);

	FTShopItemStruct Floor3Membership;
	Floor3Membership.ItemID = TEXT("3층회원권");
	Floor3Membership.Count = 1;
	Floor3Membership.Price = 0;
	Floor3Membership.bUnlockedByDefault = false;
	Floor3Membership.bFixedSlot = true;
	FixedShopItems.Add(Floor3Membership);

	const TArray<FName> DefaultRandomItemIDs =
	{
		TEXT("물"),
		TEXT("설탕"),
		TEXT("소금"),
		TEXT("비누"),
		TEXT("바게트 빵"),
		TEXT("얼음"),
		TEXT("고무줄"),
		TEXT("자이로볼"),
		TEXT("콜라"),
		TEXT("멘토스"),
		TEXT("토마토"),
		TEXT("드라이어기")
	};

	for (const FName& ItemID : DefaultRandomItemIDs)
	{
		FTShopItemStruct ShopItem;
		ShopItem.ItemID = ItemID;
		ShopItem.Count = 1;
		ShopItem.Price = 0;
		ShopItem.bUnlockedByDefault = true;
		ShopItem.bFixedSlot = false;
		RandomItemPool.Add(ShopItem);
	}
}

void AFTHubShop::BeginPlay()
{
	Super::BeginPlay();

	for (const FTShopItemStruct& ShopItem : FixedShopItems)
	{
		if (ShopItem.bUnlockedByDefault && !ShopItem.ItemID.IsNone())
		{
			UnlockedShopItemIDs.Add(ShopItem.ItemID);
		}
	}

	for (const FTShopItemStruct& ShopItem : RandomItemPool)
	{
		if (ShopItem.bUnlockedByDefault && !ShopItem.ItemID.IsNone())
		{
			UnlockedShopItemIDs.Add(ShopItem.ItemID);
		}
	}

	RefreshShopItems();
}

bool AFTHubShop::Interact_Implementation(AActor* Interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("Hub Shop Interacted"));

	OpenShopWidget(Interactor);
	return true;
}

FText AFTHubShop::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("상점 보기"));
}

void AFTHubShop::RefreshShopItems()
{
	CurrentShopItems.Reset();

	for (const FTShopItemStruct& FixedShopItem : FixedShopItems)
	{
		if (!FixedShopItem.ItemID.IsNone())
		{
			CurrentShopItems.Add(FixedShopItem);
		}
	}

	TArray<FTShopItemStruct> CandidateItems = RandomItemPool;

	const int32 SlotCount = FMath::Max(0, RandomSlotCount);
	for (int32 Index = 0; Index < SlotCount && CandidateItems.Num() > 0; ++Index)
	{
		const int32 RandomIndex = FMath::RandRange(0, CandidateItems.Num() - 1);
		CurrentShopItems.Add(CandidateItems[RandomIndex]);
		CandidateItems.RemoveAt(RandomIndex);
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Items Refreshed: %d items"), CurrentShopItems.Num());
}

bool AFTHubShop::BuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory)
{
	const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID);

	if (!ShopItem || !PlayerInventory || !CanBuyItem(ItemID, PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Failed: %s"), *ItemID.ToString());
		return false;
	}

	if (!PlayerInventory->AddItem(ShopItem->ItemID, ShopItem->Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Buy Reward Failed: %s"), *ItemID.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Shop Buy Success: %s x%d"), *ShopItem->ItemID.ToString(), ShopItem->Count);
	return true;
}

void AFTHubShop::UnlockShopItem(FName ItemID)
{
	if (ItemID.IsNone())
	{
		return;
	}

	UnlockedShopItemIDs.Add(ItemID);
	UE_LOG(LogTemp, Warning, TEXT("Shop Item Unlocked: %s"), *ItemID.ToString());
}

bool AFTHubShop::IsShopItemUnlocked(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return false;
	}

	if (UnlockedShopItemIDs.Contains(ItemID))
	{
		return true;
	}

	for (const FTShopItemStruct& ShopItem : CurrentShopItems)
	{
		if (ShopItem.ItemID == ItemID)
		{
			return ShopItem.bUnlockedByDefault;
		}
	}

	return false;
}

bool AFTHubShop::CanBuyItem(FName ItemID, UFTInventoryComponent* PlayerInventory) const
{
	const FTShopItemStruct* ShopItem = FindCurrentShopItem(ItemID);
	return ShopItem && PlayerInventory && IsShopItemUnlocked(ItemID);
}

void AFTHubShop::GetShopItems(TArray<FTShopItemStruct>& OutShopItems) const
{
	OutShopItems = CurrentShopItems;
}

void AFTHubShop::CloseShopWidget()
{
	if (HubShopWidget && HubShopWidget->IsInViewport())
	{
		HubShopWidget->RemoveFromParent();
	}

	APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;

	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
}

void AFTHubShop::OpenShopWidget(AActor* Interactor)
{
	if (HubShopWidget && HubShopWidget->IsInViewport())
	{
		CloseShopWidget();
		return;
	}

	if (!HubShopWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("HubShopWidgetClass is not assigned."));
		PrintShopItems();
		return;
	}

	APlayerController* PlayerController = nullptr;

	if (APawn* InteractorPawn = Cast<APawn>(Interactor))
	{
		PlayerController = Cast<APlayerController>(InteractorPawn->GetController());
	}

	if (!PlayerController)
	{
		PlayerController = GetWorld()
			? GetWorld()->GetFirstPlayerController()
			: nullptr;
	}

	if (!PlayerController)
	{
		return;
	}

	UFTInventoryComponent* PlayerInventory = FindPlayerInventory(Interactor);

	if (!HubShopWidget)
	{
		HubShopWidget = CreateWidget<UFTHubShopWidget>(
			PlayerController,
			HubShopWidgetClass
		);

		if (!HubShopWidget)
		{
			return;
		}
	}

	HubShopWidget->InitializeShopWidget(this, PlayerInventory);

	if (!HubShopWidget->IsInViewport())
	{
		HubShopWidget->AddToViewport();

		PlayerController->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(HubShopWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}
}

const FTShopItemStruct* AFTHubShop::FindCurrentShopItem(FName ItemID) const
{
	for (const FTShopItemStruct& ShopItem : CurrentShopItems)
	{
		if (ShopItem.ItemID == ItemID)
		{
			return &ShopItem;
		}
	}

	return nullptr;
}

UFTInventoryComponent* AFTHubShop::FindPlayerInventory(AActor* Interactor) const
{
	if (Interactor)
	{
		if (UFTInventoryComponent* PlayerInventory = Interactor->FindComponentByClass<UFTInventoryComponent>())
		{
			return PlayerInventory;
		}
	}

	const APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;

	if (!PlayerController)
	{
		return nullptr;
	}

	if (APawn* Pawn = PlayerController->GetPawn())
	{
		if (UFTInventoryComponent* PlayerInventory = Pawn->FindComponentByClass<UFTInventoryComponent>())
		{
			return PlayerInventory;
		}
	}

	return PlayerController->FindComponentByClass<UFTInventoryComponent>();
}

void AFTHubShop::PrintShopItems() const
{
	if (CurrentShopItems.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Shop Empty"));
		return;
	}

	for (const FTShopItemStruct& ShopItem : CurrentShopItems)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Shop: %s x%d / Price %d / Unlocked %s"),
			*ShopItem.ItemID.ToString(),
			ShopItem.Count,
			ShopItem.Price,
			IsShopItemUnlocked(ShopItem.ItemID) ? TEXT("true") : TEXT("false")
		);
	}
}
