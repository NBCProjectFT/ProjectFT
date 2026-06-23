#include "FTItemActor.h"

#include "Components/StaticMeshComponent.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

AFTItemActor::AFTItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsBody"));
}

void AFTItemActor::BeginPlay()
{
	Super::BeginPlay();
	ConfigureFromItemData();
}

void AFTItemActor::InitializeFromItemData(UFTItemDataAsset* InItemData)
{
	ItemData = InItemData;
	ConfigureFromItemData();
}

void AFTItemActor::ConfigureFromItemData()
{
	UpdateAppearance();
}

bool AFTItemActor::Interact_Implementation(AActor* Interactor)
{
	if (!ItemData)
	{
		return false;
	}

	UE_LOG(LogFTItem, Log, TEXT("Picked up item: %s"), *ItemData->ItemData.ItemName.ToString());
	DestroyItem();
	return true;
}

void AFTItemActor::UpdateAppearance()
{
	if (!ItemData || ItemData->ItemData.ItemMesh.IsNull())
	{
		return;
	}

	if (UStaticMesh* LoadedMesh = ItemData->ItemData.ItemMesh.LoadSynchronous())
	{
		MeshComponent->SetStaticMesh(LoadedMesh);
	}
}

const FFTItemActionDefinition* AFTItemActor::FindActionDefinition(FGameplayTag ActionTag) const
{
	if (!ItemData || !ActionTag.IsValid())
	{
		return nullptr;
	}

	return ItemData->Actions.FindByPredicate(
		[ActionTag](const FFTItemActionDefinition& Definition)
		{
			const FGameplayTag DefinitionTag = Definition.ActionTag.IsValid()
				? Definition.ActionTag
				: TAG_FT_Weapon_Action_Primary;
			return DefinitionTag.MatchesTagExact(ActionTag);
		});
}

FTransform AFTItemActor::GetMuzzleTransform() const
{
	if (ItemData && MeshComponent && MeshComponent->DoesSocketExist(ItemData->MuzzleSocketName))
	{
		return MeshComponent->GetSocketTransform(ItemData->MuzzleSocketName, RTS_World);
	}
	return GetActorTransform();
}

void AFTItemActor::DestroyItem()
{
	Destroy();
}
