#include "FTGE_Damage.h"

#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/Message/FTGameplayTags.h"

UFTGE_Damage::UFTGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = TAG_FT_Data_Damage;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UFTAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);
	Modifiers.Add(Modifier);
}
