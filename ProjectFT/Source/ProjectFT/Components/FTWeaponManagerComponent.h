#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "FTWeaponManagerComponent.generated.h"

// 공격을 구현하기 위해 Tag에 따라서 무기에 다른 컴포넌트를 생성함
// 추후에 근접공격과 원거리 공격을 둘다 구현 할 수도 있을거 같아서 이런 구조로 만들었습니다.

class UFTAttackComponent;
class UDataTable;

UCLASS(ClassGroup = (FT), meta = (BlueprintSpawnableComponent))
class PROJECTFT_API UFTWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTWeaponManagerComponent();

	UFUNCTION(BlueprintCallable, Category = "FT|Weapon")
	void Attack();

	UFUNCTION(BlueprintCallable, Category = "FT|Weapon")
	bool SetWeaponData(FName ItemName);

	UFUNCTION(BlueprintCallable, Category = "FT|Weapon")
	void ClearWeaponData();

protected:
	virtual void BeginPlay() override;
	bool BuildAttackComponent();
	void ClearAttackComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Weapon")
	TObjectPtr<UDataTable> WeaponDataTable;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FT|Weapon")
	FName EquippedItemName = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FT|Weapon")
	TObjectPtr<UFTAttackComponent> ActiveAttackComponent;
};
