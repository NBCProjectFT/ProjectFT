#include "FTAttackSecurityAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/Effects/FTGE_Damage.h"
#include "ProjectFT/Core/FTLogChannels.h"

AFTAttackSecurityAIController::AFTAttackSecurityAIController()
{
	bCanBeCaptureLeader = false;
	DamageEffectClass = UFTGE_Damage::StaticClass();
}

bool AFTAttackSecurityAIController::StartAttack()
{
	if (!CanStartAttack())
	{
		return false;
	}

	bIsAttacking = true;
	bAttackCompleted = false;

	if (bLogSecurityEventDebug)
	{
		UE_LOG(LogFTSecurity, Log, TEXT("Attack Security AI '%s' started attack"), *GetName());
	}

	return true;
}

void AFTAttackSecurityAIController::FinishAttack()
{
	if (!bIsAttacking)
	{
		return;
	}

	bIsAttacking = false;
	bAttackCompleted = true;
	LastAttackFinishedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (bLogSecurityEventDebug)
	{
		UE_LOG(LogFTSecurity, Log, TEXT("Attack Security AI '%s' finished attack"), *GetName());
	}
}

bool AFTAttackSecurityAIController::CanStartAttack() const
{
	if (!TargetActor || bIsAttacking || bTargetCaptured || bIsStunned || !bSecurityChaseActive)
	{
		return false;
	}

	if (!bIsTargetInAttackRange)
	{
		return false;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return CurrentTime - LastAttackFinishedTime >= AttackCooldown;
}

bool AFTAttackSecurityAIController::ApplyAttackDamage()
{
	APawn* ControlledPawn = GetPawn();
	if (bIsStunned || !ControlledPawn || !TargetActor || !DamageEffectClass || AttackDamage <= 0.0f)
	{
		return false;
	}

	const IAbilitySystemInterface* ControlledAbilitySystemInterface = Cast<IAbilitySystemInterface>(ControlledPawn);
	const UAbilitySystemComponent* ControlledASC = ControlledAbilitySystemInterface
		? ControlledAbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;
	if (ControlledASC && ControlledASC->HasMatchingGameplayTag(TAG_FT_State_Debuff_Immobilized))
	{
		return false;
	}

	const float CurrentDistance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	if (CurrentDistance > AttackRange)
	{
		if (bLogSecurityEventDebug)
		{
			UE_LOG(
				LogFTSecurity,
				Log,
				TEXT("Attack Security AI '%s' skipped damage: target out of range %.1f / %.1f"),
				*GetName(),
				CurrentDistance,
				AttackRange
			);
		}

		return false;
	}

	IAbilitySystemInterface* SourceAbilitySystemInterface = Cast<IAbilitySystemInterface>(ControlledPawn);
	IAbilitySystemInterface* TargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* SourceASC = SourceAbilitySystemInterface
		? SourceAbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;
	UAbilitySystemComponent* TargetASC = TargetAbilitySystemInterface
		? TargetAbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;

	if (!SourceASC || !TargetASC)
	{
		if (bLogSecurityEventDebug)
		{
			UE_LOG(
				LogFTSecurity,
				Warning,
				TEXT("Attack Security AI '%s' failed damage: SourceASC=%s TargetASC=%s Target=%s"),
				*GetName(),
				SourceASC ? TEXT("true") : TEXT("false"),
				TargetASC ? TEXT("true") : TEXT("false"),
				*GetNameSafe(TargetActor)
			);
		}

		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(ControlledPawn, ControlledPawn);
	EffectContext.AddSourceObject(ControlledPawn);

	FGameplayEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);
	if (!DamageSpec.IsValid())
	{
		return false;
	}

	// UFTGE_Damage는 Data.Damage 값을 Health에 더하므로 데미지는 음수로 전달한다.
	DamageSpec.Data->SetSetByCallerMagnitude(TAG_FT_Data_Damage, -AttackDamage);
	const FActiveGameplayEffectHandle AppliedHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

	if (bLogSecurityEventDebug)
	{
		UE_LOG(
			LogFTSecurity,
			Log,
			TEXT("Attack Security AI '%s' applied %.1f damage to %s"),
			*GetName(),
			AttackDamage,
			*GetNameSafe(TargetActor)
		);
	}

	return AppliedHandle.IsValid();
}
