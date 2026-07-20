#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "GameplayTagContainer.h"
#include "FTDamageTextSubsystem.generated.h"

class UUserWidget;
class UTextBlock;
class UImage;
class UWidgetAnimation;
class UMaterialInterface;
struct FFTCharacterDamagePayloadStruct;
struct FFTDamageTextPayloadStruct;

USTRUCT()
struct FFTDamageTextPoolItemStruct
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UUserWidget> Widget = nullptr;

	FVector WorldLocation = FVector::ZeroVector;

	bool bInUse = false;

	FTimerHandle HideTimerHandle;
};

UCLASS()
class PROJECTFT_API UFTDamageTextSubsystem
	: public ULocalPlayerSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

public:
	void ShowDamageText(float Damage, FVector HitLocation);
	void ShowFloatingText(const FText& Text, FVector HitLocation);

private:
	UUserWidget* AcquireDamageTextWidget(int32& OutPoolIndex);
	UUserWidget* CreateDamageTextWidget() const;

private:
	void UpdateDamageTextScreenPositions();

private:
	UTextBlock* FindDamageTextBlock(UUserWidget* Widget) const;
	void SetDamageText(UUserWidget* Widget, const FText& Text) const;
	void ApplyDamageTextBackgroundMaterial(UUserWidget* Widget) const;
	UWidgetAnimation* FindWidgetAnimation(UUserWidget* Widget, FName AnimationName) const;

private:
	void RegisterDamageMessageListeners();
	void UnregisterDamageMessageListeners();

	void HandleCharacterDamaged(FGameplayTag Channel, const FFTCharacterDamagePayloadStruct& Payload);
	void HandleDamageTextMessage(FGameplayTag Channel, const FFTDamageTextPayloadStruct& Payload);
	bool ShouldShowDamageTextForLocalPlayer(const FFTCharacterDamagePayloadStruct& Payload) const;
	bool ShouldShowDamageTextForLocalPlayer(AActor* InstigatorActor, AActor* TargetActor) const;

private:
	void HideDamageText(int32 PoolIndex);
	void HideAllDamageTexts();

private:
	APlayerController* GetOwningPlayerController() const;

private:
	UPROPERTY()
	TSubclassOf<UUserWidget> DamageTextWidgetClass;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> DamageTextBackgroundMaterial = nullptr;

	UPROPERTY()
	TArray<FFTDamageTextPoolItemStruct> DamageTextPool;

private:
	FGameplayMessageListenerHandle CharacterDamagedListenerHandle;
	FGameplayMessageListenerHandle DamageTextListenerHandle;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Damage Text")
	int32 InitialPoolSize = 20;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Text")
	float DisplayDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Text")
	FVector WorldOffset = FVector(0.0f, 0.0f, 80.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Damage Text")
	FName DamageTextBlockName = TEXT("TXT_Damage");

	UPROPERTY(EditDefaultsOnly, Category = "Damage Text")
	FName FadeAnimationName = TEXT("AlphaToZero");

	UPROPERTY(EditDefaultsOnly, Category = "Damage Text")
	int32 ViewportZOrder = 10;
};
