// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FTInteractable.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType, Blueprintable)
class UFTInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 라인트레이스로 감지되어 상호작용할 수 있는 대상(문/선반/아이템/제작대 등)이 구현한다.
 * BP로도 구현할 수 있게 BlueprintNativeEvent로 노출하므로, 호출 측은 직접 Cast 대신
 * Implements<UFTInteractable>() 확인 후 IFTInteractable::Execute_*() 경로로 호출한다.
 */
class PROJECTFT_API IFTInteractable
{
	GENERATED_BODY()

public:
	/** 상호작용 실행. Interactor는 상호작용을 시도한 액터(보통 플레이어 캐릭터). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FT|Interaction")
	void Interact(AActor* Interactor);
	virtual void Interact_Implementation(AActor* Interactor) {}

	/** UI 프롬프트에 표시할 텍스트(예: "열기", "줍기"). 포커스될 때 사용한다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FT|Interaction")
	FText GetInteractionPrompt() const;
	virtual FText GetInteractionPrompt_Implementation() const { return FText::GetEmpty(); }
};
