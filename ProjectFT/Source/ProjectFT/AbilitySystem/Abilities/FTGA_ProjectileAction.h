
#pragma once

#include "CoreMinimal.h"
#include "FTGA_ItemAbility.h"
#include "FTGA_ProjectileAction.generated.h"

class UFTItemDataAsset;
class UFTProjectileActorDataAsset;

/**
 * 투사체가 대상에게 적중했을 때 효과를 적용하는 어빌리티다.
 *
 * 직접 입력으로 실행되는 공격 어빌리티가 아니라, AFTProjectileActor가 충돌/폭발을 감지한 뒤
 * Event.TargetHit 이벤트를 소유자의 ASC로 보내면 실행된다.
 * Payload.OptionalObject에는 UFTProjectileActorDataAsset이 들어오고,
 * Payload.Target에는 실제 효과를 받을 대상 액터가 들어온다.
 */
UCLASS()
class PROJECTFT_API UFTGA_ProjectileAction : public UFTGA_ItemAbility
{
	GENERATED_BODY()
	
public:
	UFTGA_ProjectileAction();

	// ProjectileActor가 보낸 TargetHit 이벤트를 받아 TargetActor에게 UseEffects를 적용한다.
	virtual void ActivateAbility
	(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData
		) override;
	
private:
	// 이번 적중 이벤트에서 사용 중인 ProjectileActorDataAsset의 부모 아이템 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UFTItemDataAsset> ActiveItemData = nullptr;
	
	// 충돌/폭발 세부 설정과 적용할 UseEffects를 가진 투사체 데이터.
	UPROPERTY(Transient)
	TObjectPtr<UFTProjectileActorDataAsset> ProjectileActorData = nullptr;
};
