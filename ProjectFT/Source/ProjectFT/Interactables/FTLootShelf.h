// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectFT/Interface/FTInteractable.h"
#include "ProjectFT/Interface/FTDamageable.h"
#include "FTLootShelf.generated.h"

class UStaticMeshComponent;
class UFTChanneledInteractionComponent;

/**
 * 마트 진열대에서 물건을 "훔치는" 채널형 상호작용 대상(테스트/예시).
 * 상호작용 키를 꾹 누르면 게이지가 차고 중간중간 스킬체크가 뜨며, 게이지가 가득 차면 훔치기 완료로 처리한다.
 * 채널 로직은 UFTChanneledInteractionComponent가, 포커스/프롬프트는 IFTInteractable이 담당한다.
 */
UCLASS()
class PROJECTFT_API AFTLootShelf : public AActor, public IFTInteractable, public IFTDamageable
{
	GENERATED_BODY()

public:
	AFTLootShelf();

	//~ Begin IFTInteractable
	// 즉시 상호작용(Interact)은 사용하지 않는다(채널형이라 누르면 채널링이 시작됨). 프롬프트만 제공한다.
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IFTInteractable

	//~ Begin IFTDamageable
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	//~ End IFTDamageable

protected:
	virtual void BeginPlay() override;

	// 채널형 상호작용 완료(게이지 가득 참) 시 호출 — 훔치기 성공 처리.
	UFUNCTION()
	void HandleStealCompleted();

	// [아이템 획득 방법 1] 인벤토리에 직접 아이템을 넣어주는 함수 (메시지 전송)
	void GiveStealReward();

	// [아이템 획득 방법 2] 파괴 시 바닥에 아이템을 뿌리는 함수 (액터 스폰)
	void DropItemsOnFloor();

	void TestCode();
protected:
	// 진열대 메시(루트). 상호작용 트레이스(Visibility)에 잡히도록 콜리전이 있어야 한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Shelf", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ShelfMesh;

	// 꾹 눌러 훔치는 채널형 상호작용(진행도 + 스킬체크).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Shelf", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFTChanneledInteractionComponent> ChanneledInteraction;

	// 포커스 시 UI에 표시할 프롬프트 텍스트.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Shelf")
	FText InteractionPrompt = FText::FromString(TEXT("훔치기"));

	// 완료 시 액터를 제거할지(테스트용: 훔치면 진열대가 사라짐).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Shelf")
	bool bDestroyOnComplete = true;

	// 보상 아이템 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Shelf|Loot")
	TObjectPtr<class UFTItemDataAsset> LootItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Shelf|Loot")
	int32 LootQuantity = 3;

	// 매대 내구도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FT|Shelf|Status")
	float Health = 30.0f;

	// 중복 획득 방지
	bool bHasBeenLooted = false;
};