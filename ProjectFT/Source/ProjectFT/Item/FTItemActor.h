#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "FTItemActor.generated.h"

class UFTItemDataAsset;
class UStaticMeshComponent;
class UWidgetComponent;

UCLASS()
class PROJECTFT_API AFTItemActor : public AActor, public IFTInteractable
{
	GENERATED_BODY()

public:
	AFTItemActor();

	/* @brief : 아이템의 외형을 데이터 에셋에 맞춰 업데이트 하는 메서드입니다. */
	void UpdateAppearance();
	
	/* @brief : 아이템 위에 띄워진 툴팁 UI의 가시성과 정보를 설정합니다. */
	void SetTooltipVisibility(bool bVisible);

	/* @brief : 물리 및 콜리전 설정을 초기화하는 메서드입니다. (생성 및 풀 재사용 시 사용) */
	void SetupPhysicsAndCollision();

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
	
	// 머리 위에 아이템 미리보기를 띄울 위젯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> TooltipWidgetComponent;

	// 에디터에서 직접 툴팁 크기와 위치를 조정할 수 있게 변수로 노출합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|UI")
	FVector2D TooltipDrawSize = FVector2D(220.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|UI")
	FVector TooltipRelativeLocation = FVector(0.0f, 0.0f, 120.0f); // 기본 80cm에서 120cm로 높게 기본값 변경
};