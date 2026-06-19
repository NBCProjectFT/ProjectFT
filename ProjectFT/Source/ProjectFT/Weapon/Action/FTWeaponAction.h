#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectFT/Struct/FTWeaponActionDefinition.h"
#include "FTWeaponAction.generated.h"

class UFTWeaponActionComponent;

/** Lightweight runtime command owned by a weapon action component. */
UCLASS(Abstract)
class PROJECTFT_API UFTWeaponAction : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UFTWeaponActionComponent* InActionComponent,
		const FFTWeaponActionDefinition& InDefinition);
	bool StartAction();
	virtual void NotifyWindowBegin() {}
	virtual void NotifyWindowTick() {}
	virtual void NotifyWindowEnd() {}

	virtual UWorld* GetWorld() const override;

protected:
	bool PlayAttackMontage() const;
	virtual bool ExecuteAction() PURE_VIRTUAL(UFTWeaponAction::ExecuteAction, return false;);

	UPROPERTY(Transient)
	TObjectPtr<UFTWeaponActionComponent> ActionComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FT|Weapon")
	FFTWeaponActionDefinition Definition;
	double LastExecutionTime = -DBL_MAX;
};
