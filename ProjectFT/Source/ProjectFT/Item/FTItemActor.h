#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTItemActor.generated.h"

class UFTItemDataAsset;
class UFTProjectileComponent;
class UStaticMeshComponent;

UCLASS()
class PROJECTFT_API AFTItemActor : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTItemActor();
	UStaticMeshComponent* GetItemMeshComponent() const { return MeshComponent; }

	/*
	 * @brief : 아이템의 외형을 데이터 에셋에 맞춰 업데이트 하는 메서드입니다.
	 */
	void InitializeFromItemData(UFTItemDataAsset* InItemData);
	void SetEquipped(bool bEquipped);
	void UpdateAppearance();

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
