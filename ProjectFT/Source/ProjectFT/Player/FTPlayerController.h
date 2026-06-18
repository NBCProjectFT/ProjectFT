// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FTPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class IFTInputInterface;
struct FInputActionValue;

/**
 * 입력 소유 주체. Mapping Context를 등록하고 IA를 바인딩한 뒤,
 * 입력 값을 IFTLocomotionInput을 구현한 Pawn으로 전달한다.
 * 실제 Move/Look/Jump 로직 실행은 Pawn(Character)이 담당한다.
 * 축 교환/반전 같은 입력 가공은 IMC의 Input Modifier에서 처리한다(여기서 하지 않는다).
 */
UCLASS()
class PROJECTFT_API AFTPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	// 등록할 기본 Input Mapping Context. BP_PlayerController에서 IMC_Player를 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// 등록 전에 기존 mapping을 모두 비울지 여부(다른 IMC가 남아 입력이 중복되는 문제 방지).
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

private:
	// 현재 빙의 중인 Pawn(GC 추적용).
	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedInputPawn;

	// CachedInputPawn에서 한 번만 해석한 입력 수신자. Pawn 수명에 종속된다.
	IFTInputInterface* CachedLocomotionInput = nullptr;
};
