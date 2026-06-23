#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTItemActor.generated.h"

class UFTItemDataAsset;
class UStaticMeshComponent;
struct FFTItemActionDefinition;

UCLASS()
class PROJECTFT_API AFTItemActor : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTItemActor();
	void InitializeFromItemData(UFTItemDataAsset* InItemData);
	const FFTItemActionDefinition* FindActionDefinition(FGameplayTag ActionTag) const;
	FTransform GetMuzzleTransform() const;
	UStaticMeshComponent* GetItemMeshComponent() const { return MeshComponent; }

	/*
	 * @brief : 아이템의 외형을 데이터 에셋에 맞춰 업데이트 하는 메서드입니다.
	 */
	void UpdateAppearance();
	void ConfigureFromItemData();

protected:
	virtual void BeginPlay() override;
	
	// IFTInteractable 인터페이스 구현
	virtual bool Interact_Implementation(AActor* Interactor) override;

	/*
	 * @brief : 아이템 제거 메서드입니다.
	 */
	virtual void DestroyItem();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UFTItemDataAsset> ItemData;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};
