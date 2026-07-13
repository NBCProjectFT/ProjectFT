#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FTShoppingPoint.generated.h"

class UArrowComponent;
class UBoxComponent;

UCLASS()
class PROJECTFT_API AFTShoppingPoint : public AActor
{
	GENERATED_BODY()

public:
	AFTShoppingPoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Shopping")
	TObjectPtr<UBoxComponent> ShoppingAreaBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Shopping")
	TObjectPtr<UArrowComponent> LookDirectionArrow;

	/** 쇼핑 목적지를 뽑을 박스 영역의 절반 크기다. X/Y는 바닥 영역, Z는 NavMesh 투영 여유 높이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Shopping", meta = (ClampMin = "0.0"))
	FVector BoxExtent = FVector(300.0f, 200.0f, 100.0f);

	/** 이 쇼핑포인트가 선택될 확률 가중치다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Shopping", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	/** 동시에 이 쇼핑포인트를 선택할 수 있는 손님 수다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Shopping", meta = (ClampMin = "1"))
	int32 MaxSelectors = 3;

	/** Move To에서 도착으로 인정할 거리다. StateTree에서 필요하면 이 값을 바인딩한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Shopping", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 100.0f;

	/** 쇼핑 영역 박스 디버그 표시 여부다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Shopping|Debug")
	bool bDrawDebugArea = true;

	/** 현재 이 쇼핑포인트를 선택한 손님 수다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FT|Shopping")
	int32 CurrentSelectors = 0;

	bool CanReserve() const;
	bool TryReserve();
	void Release();
	bool GetRandomShoppingLocation(UObject* WorldContextObject, FVector& OutLocation) const;
	// FVector GetShoppingLookLocation() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;
};
