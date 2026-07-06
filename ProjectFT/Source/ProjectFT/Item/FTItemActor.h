#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTItemActor.generated.h"

class UFTItemDataAsset;
class UStaticMeshComponent;

UCLASS()
class PROJECTFT_API AFTItemActor : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTItemActor();

	/* @brief : 아이템의 외형을 데이터 에셋에 맞춰 업데이트 하는 메서드입니다. */
	void UpdateAppearance();

protected:
	virtual void BeginPlay() override;
	/*
	 * @brief : IFTInteractable 인터페이스를 구현합니다.
	 * @Param Interactor : 상호작용한 플레이어 캐릭터 
	 */
	virtual bool Interact_Implementation(AActor* Interactor) override;

	/*
	 * @brief : 아이템 제거 메서드입니다.
	 * 월드가 유효한지 확인 후 풀 서브 시스템에게 파괴 권한을 양도
	 * 만약 서브 시스템이 없가나 오류 상황일 때만 파괴 실행
	 */
	virtual void DestroyItem();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UFTItemDataAsset> ItemData;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};