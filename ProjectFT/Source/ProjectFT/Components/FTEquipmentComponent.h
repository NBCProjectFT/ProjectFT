// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "FTEquipmentComponent.generated.h"

class AFTItemActor;
class UAbilitySystemComponent;
class UFTItemDataAsset;

/**
 * 테스트용 장착 컴포넌트입니다.
 *
 * FTPlayerCharacter 코드를 건드리지 않고 BP에 붙여서 사용합니다.
 * DefaultItemData가 지정되어 있으면 BeginPlay 때 아이템 액터를 생성하고,
 * 근접 아이템이면 UFTMeleeDataAsset::MeleeAttackData.AttachSocketName에 붙입니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTEquipmentComponent();

	UFUNCTION(BlueprintCallable, Category = "FT|Equipment")
	void EquipItem(UFTItemDataAsset* ItemData);

	UFUNCTION(BlueprintCallable, Category = "FT|Equipment")
	void UnequipCurrentItem();

	UFUNCTION(BlueprintCallable, Category = "FT|Equipment|Attack")
	bool TryAttack();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	TObjectPtr<UFTItemDataAsset> DefaultItemData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	TSubclassOf<AFTItemActor> ItemActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	FName FallbackAttachSocketName = TEXT("MeleeHandGrip_R");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	FVector RelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	bool bEquipDefaultOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment|Attack")
	bool bEnableTemporaryAttackInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment|Attack", meta = (EditCondition = "bEnableTemporaryAttackInput"))
	FKey TemporaryAttackKey = EKeys::LeftMouseButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment|Attack")
	bool bGrantDefaultItemAbilityOnBeginPlay = true;

private:
	FName ResolveAttachSocketName(const UFTItemDataAsset* ItemData) const;
	USkeletalMeshComponent* ResolveOwnerMesh() const;
	UAbilitySystemComponent* ResolveOwnerAbilitySystem() const;
	bool EnsureAbilityGranted(UAbilitySystemComponent* AbilitySystemComponent, UFTItemDataAsset* ItemData) const;
	bool IsLocalPlayerOwner() const;

	UPROPERTY(Transient)
	TObjectPtr<AFTItemActor> EquippedItemActor = nullptr;
};
