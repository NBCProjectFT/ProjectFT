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
 * UFTEquipmentComponent
 *
 * 테스트용 장착/공격 컴포넌트입니다.
 *
 * 목적:
 * - FTPlayerCharacter 코드를 직접 수정하지 않고,
 *   캐릭터 BP에 컴포넌트로 붙여서 아이템 장착과 임시 공격 테스트를 하기 위한 컴포넌트입니다.
 *
 * 전체 흐름:
 *
 * 1. BeginPlay에서 DefaultItemData가 있으면 아이템 액터를 생성합니다.
 * 2. 생성된 아이템 액터를 캐릭터 Mesh의 특정 소켓에 Attach합니다.
 * 3. DefaultItemData 안에 UseAbility가 있으면 ASC에 Ability를 임시로 부여합니다.
 * 4. Tick에서 임시 공격 키 입력을 감지합니다.
 * 5. 공격 키가 눌리면 TryAttack()을 호출합니다.
 * 6. TryAttack()은 ItemData의 UseAbility를 찾고,
 *    해당 Ability의 TriggerEventTag를 이용해 ASC->HandleGameplayEvent()를 호출합니다.
 * 7. GameplayEvent를 받은 GAS가 해당 Ability를 실행합니다.
 *
 * 주의:
 * - 이 컴포넌트는 디버깅/테스트용입니다.
 * - 정식 구조에서는 입력 처리는 PlayerController/InputComponent 쪽으로 이동하는 것이 좋습니다.
 * - 장착/해제도 나중에는 Inventory/Equipment 시스템으로 분리하는 것이 좋습니다.
 */

// 디버깅 용이기 때문에 추후에 삭제 하도록 하겠습니다.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTFT_API UFTEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFTEquipmentComponent();

	/**
	 * ItemData를 기반으로 아이템을 장착합니다.
	 *
	 * 동작 방식:
	 * - 기존에 장착된 아이템이 있으면 먼저 제거합니다.
	 * - ItemActorClass를 Spawn합니다.
	 * - Spawn된 ItemActor에 ItemData를 넣습니다.
	 * - ItemActor의 외형을 ItemData 기준으로 갱신합니다.
	 * - 아이템 액터의 충돌/물리를 끕니다.
	 * - 캐릭터 Mesh의 장착 소켓에 Attach합니다.
	 *
	 * ItemData가 nullptr이면 현재 장착 아이템을 해제합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Equipment")
	void EquipItem(UFTItemDataAsset* ItemData);

	/**
	 * 현재 장착 중인 아이템 액터를 제거합니다.
	 *
	 * 동작 방식:
	 * - EquippedItemActor가 있으면 Destroy합니다.
	 * - 포인터를 nullptr로 초기화합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Equipment")
	void UnequipCurrentItem();

	/**
	 * 현재 DefaultItemData를 사용해 임시 공격을 시도합니다.
	 *
	 * 동작 방식:
	 * - DefaultItemData에서 UseAbility를 찾습니다.
	 * - Owner의 AbilitySystemComponent를 찾습니다.
	 * - 해당 Ability가 아직 부여되어 있지 않다면 GiveAbility로 부여합니다.
	 * - Ability CDO에서 TriggerEventTag를 가져옵니다.
	 * - CooldownTag가 붙어 있으면 공격을 막습니다.
	 * - ASC->HandleGameplayEvent()로 Ability를 실행시킵니다.
	 *
	 * 반환값:
	 * - true: GameplayEvent로 하나 이상의 Ability가 활성화됨
	 * - false: Ability 실행 실패
	 */
	UFUNCTION(BlueprintCallable, Category = "FT|Equipment|Attack")
	bool TryAttack();

protected:
	virtual void BeginPlay() override;

	/**
	 * 테스트용 임시 입력 처리 Tick입니다.
	 *
	 * bEnableTemporaryAttackInput이 true이고,
	 * Owner가 로컬 플레이어일 때만 입력을 검사합니다.
	 *
	 * 정식 구조에서는 Tick에서 입력을 확인하기보다,
	 * Enhanced Input / PlayerController / Character Input Binding 쪽으로 옮기는 것이 좋습니다.
	 */
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	/**
	 * BeginPlay 때 자동으로 장착할 기본 아이템 데이터입니다.
	 *
	 * 예:
	 * - 테스트용 검 DataAsset
	 * - 테스트용 총 DataAsset
	 * - 테스트용 주먹/근접 아이템 DataAsset
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	TObjectPtr<UFTItemDataAsset> DefaultItemData = nullptr;

	/**
	 * 실제 월드에 Spawn할 아이템 액터 클래스입니다.
	 *
	 * 기본값은 AFTItemActor::StaticClass()입니다.
	 * 특별한 장착 액터가 필요하면 BP에서 다른 클래스로 바꿀 수 있습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	TSubclassOf<AFTItemActor> ItemActorClass;

	/**
	 * ItemData에서 장착 소켓을 찾지 못했을 때 사용할 기본 소켓 이름입니다.
	 *
	 * 예:
	 * - MeleeHandGrip_R
	 * - Weapon_R
	 * - hand_rSocket
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	FName FallbackAttachSocketName = TEXT("MeleeHandGrip_R");

	/**
	 * 아이템을 소켓에 Attach한 뒤 적용할 상대 위치 보정값입니다.
	 *
	 * 소켓에 붙였는데 위치가 살짝 어긋나는 경우 BP에서 조정합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	FVector RelativeLocation = FVector::ZeroVector;

	/**
	 * 아이템을 소켓에 Attach한 뒤 적용할 상대 회전 보정값입니다.
	 *
	 * 손에 붙었지만 방향이 틀어진 경우 BP에서 조정합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	/**
	 * 아이템을 소켓에 Attach한 뒤 적용할 상대 스케일 보정값입니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	FVector RelativeScale = FVector::OneVector;

	/**
	 * true이면 BeginPlay에서 DefaultItemData를 자동 장착합니다.
	 *
	 * 테스트용:
	 * - 캐릭터 BP에 컴포넌트를 붙이고
	 * - DefaultItemData만 넣어두면
	 * - 시작과 동시에 아이템이 손에 붙습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment")
	bool bEquipDefaultOnBeginPlay = true;

	/**
	 * true이면 Tick에서 TemporaryAttackKey 입력을 검사합니다.
	 *
	 * 테스트용 임시 입력입니다.
	 * 정식 구조에서는 Enhanced Input으로 교체하는 것이 좋습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment|Attack")
	bool bEnableTemporaryAttackInput = true;

	/**
	 * 임시 공격 입력 키입니다.
	 *
	 * bEnableTemporaryAttackInput이 true일 때만 의미가 있습니다.
	 *
	 * 기본값:
	 * - LeftMouseButton
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment|Attack", meta = (EditCondition = "bEnableTemporaryAttackInput"))
	FKey TemporaryAttackKey = EKeys::LeftMouseButton;

	/**
	 * true이면 BeginPlay에서 DefaultItemData의 UseAbility를 ASC에 미리 부여합니다.
	 *
	 * false여도 TryAttack() 안에서 EnsureAbilityGranted()를 다시 호출하므로,
	 * 공격 시점에 Ability가 부여될 수 있습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Equipment|Attack")
	bool bGrantDefaultItemAbilityOnBeginPlay = true;

private:
	/**
	 * ItemData 기준으로 장착할 소켓 이름을 결정합니다.
	 *
	 * 현재 로직:
	 * - ItemData가 UFTMeleeDataAsset이면
	 *   MeleeAttackData.AttachSocketName을 우선 사용합니다.
	 * - 값이 비어 있거나 근접 데이터가 아니면
	 *   FallbackAttachSocketName을 사용합니다.
	 */
	FName ResolveAttachSocketName(const UFTItemDataAsset* ItemData) const;

	/**
	 * Owner에서 장착 대상 Mesh를 찾습니다.
	 *
	 * 현재 로직:
	 * - Owner가 ACharacter이면 Character->GetMesh() 사용
	 * - Character가 아니면 Owner 안에서 USkeletalMeshComponent를 검색
	 */
	USkeletalMeshComponent* ResolveOwnerMesh() const;

	/**
	 * Owner가 가지고 있는 AbilitySystemComponent를 찾습니다.
	 *
	 * UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent()를 사용하므로,
	 * Owner가 IAbilitySystemInterface를 구현하고 있거나,
	 * ASC를 적절히 노출하고 있어야 합니다.
	 */
	UAbilitySystemComponent* ResolveOwnerAbilitySystem() const;

	/**
	 * ItemData에 지정된 UseAbility가 ASC에 부여되어 있는지 확인하고,
	 * 없으면 GiveAbility로 부여합니다.
	 *
	 * 반환값:
	 * - true: 이미 부여되어 있거나 새로 부여함
	 * - false: ASC / ItemData / UseAbility 중 하나가 없음
	 *
	 * 주의:
	 * - 멀티플레이에서는 GiveAbility는 보통 서버 권한에서만 해야 합니다.
	 * - 현재 컴포넌트는 테스트용이므로 단순하게 처리합니다.
	 */
	bool EnsureAbilityGranted(
		UAbilitySystemComponent* AbilitySystemComponent,
		UFTItemDataAsset* ItemData
	) const;

	/**
	 * Owner가 로컬 플레이어가 조종 중인지 확인합니다.
	 *
	 * Tick에서 임시 입력을 로컬 플레이어에게만 처리하기 위해 사용합니다.
	 */
	bool IsLocalPlayerOwner() const;

	/**
	 * 현재 장착되어 있는 아이템 액터입니다.
	 *
	 * Transient:
	 * - 저장 대상이 아닙니다.
	 * - 런타임 중에만 유지됩니다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<AFTItemActor> EquippedItemActor = nullptr;
};