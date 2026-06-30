// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FTPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class IFTInputInterface;
struct FInputActionValue;

UCLASS()
class PROJECTFT_API AFTPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// 기존 매핑 초기화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	bool bClearMappingsBeforeAdd = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> SkillCheckAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> UseItemAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> QuickSlot1Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> QuickSlot2Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> QuickSlot3Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> QuickSlot4Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> QuickSlot5Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> QuickSlot6Action;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;

private:
	// Enhanced Input 콜백. 받은 값을 가공 없이 입력 수신자로 넘긴다.
	void OnMoveTriggered(const FInputActionValue& Value);
	void OnLookTriggered(const FInputActionValue& Value);
	void OnJumpStarted(const FInputActionValue& Value);
	void OnJumpCompleted(const FInputActionValue& Value);
	void OnSprintStarted(const FInputActionValue& Value);
	void OnSprintCompleted(const FInputActionValue& Value);
	void OnCrouchStarted(const FInputActionValue& Value);
	void OnCrouchCompleted(const FInputActionValue& Value);
	void OnInteractStarted(const FInputActionValue& Value);
	void OnInteractCompleted(const FInputActionValue& Value);
	void OnSkillCheckStarted(const FInputActionValue& Value);
	void OnUseItemStarted(const FInputActionValue& Value);
	void OnUseItemCompleted(const FInputActionValue& Value);
	void OnQuickSlot1Started(const FInputActionValue& Value);
	void OnQuickSlot2Started(const FInputActionValue& Value);
	void OnQuickSlot3Started(const FInputActionValue& Value);
	void OnQuickSlot4Started(const FInputActionValue& Value);
	void OnQuickSlot5Started(const FInputActionValue& Value);
	void OnQuickSlot6Started(const FInputActionValue& Value);
	void ToggleInventoryStarted(const FInputActionValue& Value);

	// [Temp/Debug] IA 에셋/IMC 매핑 없이 테이저를 테스트하기 위한 핸들러 — 퀵슬롯0 선택 후 사용(T 키에 바인딩).
	void OnDebugUseQuickSlot0();
	
	
	
	
	// 현재 빙의 중인 Pawn(GC 추적용).
	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedInputPawn;

	// CachedInputPawn에서 한 번만 해석한 입력 수신자. Pawn 수명에 종속된다.
	IFTInputInterface* CachedLocomotionInput = nullptr;
};
