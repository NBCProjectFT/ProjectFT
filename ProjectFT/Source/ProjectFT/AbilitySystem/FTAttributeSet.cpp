// Fill out your copyright notice in the Description page of Project Settings.

#include "FTAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTCharacterDamagePayloadStruct.h"

UFTAttributeSet::UFTAttributeSet()
{
	// 스탯 기본값 초기화(이전 핸드롤 스탯의 기본값과 동일).
	InitHealth(3.0f);
	InitMaxHealth(5.0f);
	InitMoveSpeed(600.0f);
}

void UFTAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UFTAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attr = Data.EvaluatedData.Attribute;

	if (Attr == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		const float DamageAmount = -Data.EvaluatedData.Magnitude;
		if (DamageAmount > 0.0f)
		{
			AActor* TargetActor = Data.Target.GetAvatarActor();
			AActor* InstigatorActor = Data.EffectSpec.GetContext().GetOriginalInstigator();
			if (!InstigatorActor)
			{
				InstigatorActor = Data.EffectSpec.GetContext().GetEffectCauser();
			}
			if (!InstigatorActor)
			{
				InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigator();
			}
			if (!InstigatorActor)
			{
				InstigatorActor = Data.EffectSpec.GetEffectContext().GetInstigatorAbilitySystemComponent()
					? Data.EffectSpec.GetEffectContext().GetInstigatorAbilitySystemComponent()->GetAvatarActor()
					: nullptr;
			}

			if (TargetActor)
			{
				FFTCharacterDamagePayloadStruct Payload;
				Payload.InstigatorActor = InstigatorActor;
				Payload.TargetActor = TargetActor;
				Payload.DamageAmount = DamageAmount;
				Payload.HitLocation = TargetActor->GetActorLocation();
				Payload.bTargetKnockedOut = GetHealth() <= 0.0f;

				UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(TargetActor->GetWorld());
				MessageSubsystem.BroadcastMessage(TAG_FT_Event_CharacterDamaged, Payload);

				if (Payload.bTargetKnockedOut)
				{
					MessageSubsystem.BroadcastMessage(TAG_FT_Event_CharacterKnockedOut, Payload);
				}
			}
		}

		if (GetHealth() <= 0.0f)
		{
			OnOutOfHealth.Broadcast();
		}
	}
	else if (Attr == GetMaxHealthAttribute())
	{
		// 최대 체력이 줄면 현재값을 끌어내린다.
		SetHealth(FMath::Min(GetHealth(), GetMaxHealth()));
	}
}
