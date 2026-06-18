#pragma once
#include "Coreminimal.h"
#include "UObject/Interface.h"
#include "FTInteractable.generated.h"

UINTERFACE(MinimalAPI)
class UFTInteractable : public UInterface
{
	GENERATED_BODY()	
};

class PROJECTFT_API IFTInteractable
{
	GENERATED_BODY()
	
public:
	/*
	 * @brief : 플레이어가 아이템을 줍거나, 매대와 상호작용할 때 사용할 메서드입니다.
	 * @Param Instigator : 상호작용을 시도하는 액더(ex: 플레이어)
	 * @return : 상호작용 성공 여부
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool Interact(AActor* InInstigator);
};
