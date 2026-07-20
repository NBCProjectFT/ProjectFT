#include "FTDamageTextSubsystem.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "NativeGameplayTags.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Manager/AssetManager/FTAssetManager.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTCharacterDamagePayloadStruct.h"
#include "ProjectFT/Struct/FTDamageTextPayloadStruct.h"

namespace FTDamageTextMessageTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_FT_Event_DamageText, "Event.Damage.Text");
}

void UFTDamageTextSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	DamageTextWidgetClass = UFTAssetManager::Get().GetDamageTextWidgetClass();
	DamageTextBackgroundMaterial = UFTAssetManager::Get().GetDamageTextBackgroundMaterial();
	if (!DamageTextWidgetClass)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Damage text widget class is not set in UI data."));
	}

	RegisterDamageMessageListeners();

	for (int32 Index = 0; Index < InitialPoolSize; ++Index)
	{
		if (UUserWidget* Widget = CreateDamageTextWidget())
		{
			FFTDamageTextPoolItemStruct& PoolItem = DamageTextPool.AddDefaulted_GetRef();
			PoolItem.Widget = Widget;
			PoolItem.WorldLocation = FVector::ZeroVector;
			PoolItem.bInUse = false;
		}
	}
}

void UFTDamageTextSubsystem::Deinitialize()
{
	UnregisterDamageMessageListeners();
	HideAllDamageTexts();

	Super::Deinitialize();
}

void UFTDamageTextSubsystem::Tick(float DeltaTime)
{
	UpdateDamageTextScreenPositions();
}


TStatId UFTDamageTextSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFTDamageTextSubsystem, STATGROUP_Tickables);
}

bool UFTDamageTextSubsystem::IsTickable() const
{
	return !IsTemplate() && GetWorld() != nullptr && DamageTextPool.Num() > 0;
}

void UFTDamageTextSubsystem::ShowDamageText(float Damage, FVector HitLocation)
{
	ShowFloatingText(FText::AsNumber(FMath::RoundToInt(Damage)), HitLocation);
}

void UFTDamageTextSubsystem::ShowFloatingText(const FText& Text, FVector HitLocation)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	int32 PoolIndex = INDEX_NONE;
	UUserWidget* Widget = AcquireDamageTextWidget(PoolIndex);
	if (!Widget || PoolIndex == INDEX_NONE)
	{
		return;
	}

	FFTDamageTextPoolItemStruct& PoolItem = DamageTextPool[PoolIndex];

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PoolItem.HideTimerHandle);
	}

	PoolItem.WorldLocation = HitLocation + WorldOffset;
	PoolItem.bInUse = true;

	SetDamageText(Widget, Text);

	Widget->StopAllAnimations();
	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

	FVector2D InitialWidgetPosition;
	const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PlayerController,
		PoolItem.WorldLocation,
		InitialWidgetPosition,
		true
	);

	if (bProjected)
	{
		Widget->SetPositionInViewport(InitialWidgetPosition, false);
	}
	else
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
	}

	float WidgetDuration = DisplayDuration;
	if (UWidgetAnimation* Animation = FindWidgetAnimation(Widget, FadeAnimationName))
	{
		Widget->SetRenderOpacity(1.0f);
		Widget->PlayAnimation(Animation, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		WidgetDuration = Animation->GetEndTime() - Animation->GetStartTime();
	}

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate HideDelegate;
		HideDelegate.BindUObject(this, &ThisClass::HideDamageText, PoolIndex);

		World->GetTimerManager().SetTimer(
			PoolItem.HideTimerHandle,
			HideDelegate,
			WidgetDuration,
			false
		);
	}
}

UUserWidget* UFTDamageTextSubsystem::AcquireDamageTextWidget(int32& OutPoolIndex)
{
	for (int32 Index = 0; Index < DamageTextPool.Num(); ++Index)
	{
		FFTDamageTextPoolItemStruct& PoolItem = DamageTextPool[Index];
		if (!PoolItem.bInUse && PoolItem.Widget)
		{
			OutPoolIndex = Index;
			PoolItem.bInUse = true;
			return PoolItem.Widget;
		}
	}

	if (UUserWidget* Widget = CreateDamageTextWidget())
	{
		FFTDamageTextPoolItemStruct& PoolItem = DamageTextPool.AddDefaulted_GetRef();
		PoolItem.Widget = Widget;
		PoolItem.WorldLocation = FVector::ZeroVector;
		PoolItem.bInUse = true;

		OutPoolIndex = DamageTextPool.Num() - 1;
		return Widget;
	}

	OutPoolIndex = INDEX_NONE;
	return nullptr;
}

UUserWidget* UFTDamageTextSubsystem::CreateDamageTextWidget() const
{
	if (!DamageTextWidgetClass)
	{
		return nullptr;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return nullptr;
	}

	UUserWidget* Widget = CreateWidget<UUserWidget>(
		PlayerController,
		DamageTextWidgetClass
	);

	if (!Widget)
	{
		return nullptr;
	}

	ApplyDamageTextBackgroundMaterial(Widget);

	Widget->SetVisibility(ESlateVisibility::Collapsed);
	Widget->SetIsEnabled(true);
	Widget->AddToViewport(ViewportZOrder);
	
	return Widget;
}

void UFTDamageTextSubsystem::UpdateDamageTextScreenPositions()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	for (FFTDamageTextPoolItemStruct& PoolItem : DamageTextPool)
	{
		if (!PoolItem.bInUse || !PoolItem.Widget)
		{
			continue;
		}

		FVector2D WidgetPosition;
		const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController,
			PoolItem.WorldLocation,
			WidgetPosition,
			true
		);

		if (!bProjected)
		{
			PoolItem.Widget->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		PoolItem.Widget->SetPositionInViewport(WidgetPosition, false);

		if (PoolItem.Widget->GetVisibility() == ESlateVisibility::Collapsed)
		{
			PoolItem.Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

UTextBlock* UFTDamageTextSubsystem::FindDamageTextBlock(UUserWidget* Widget) const
{
	if (!Widget || !Widget->WidgetTree)
	{
		return nullptr;
	}

	if (UTextBlock* DamageTextBlock = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(DamageTextBlockName)))
	{
		return DamageTextBlock;
	}

	TArray<UWidget*> AllWidgets;
	Widget->WidgetTree->GetAllWidgets(AllWidgets);

	for (UWidget* ChildWidget : AllWidgets)
	{
		if (UTextBlock* DamageTextBlock = Cast<UTextBlock>(ChildWidget))
		{
			return DamageTextBlock;
		}
	}

	return nullptr;
}

void UFTDamageTextSubsystem::SetDamageText(UUserWidget* Widget, const FText& Text) const
{
	if (UTextBlock* DamageTextBlock = FindDamageTextBlock(Widget))
	{
		DamageTextBlock->SetText(Text);
	}
}

void UFTDamageTextSubsystem::ApplyDamageTextBackgroundMaterial(UUserWidget* Widget) const
{
	if (!Widget || !Widget->WidgetTree || !DamageTextBackgroundMaterial)
	{
		return;
	}

	if (UImage* BackgroundImage = Cast<UImage>(Widget->WidgetTree->FindWidget(TEXT("IMG_SDFJaggedDiamond"))))
	{
		BackgroundImage->SetBrushFromMaterial(DamageTextBackgroundMaterial);
	}
}

void UFTDamageTextSubsystem::RegisterDamageMessageListeners()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

	CharacterDamagedListenerHandle = MessageSubsystem.RegisterListener(
		TAG_FT_Event_CharacterDamaged,
		this,
		&ThisClass::HandleCharacterDamaged
	);

	DamageTextListenerHandle = MessageSubsystem.RegisterListener(
		FTDamageTextMessageTags::TAG_FT_Event_DamageText,
		this,
		&ThisClass::HandleDamageTextMessage
	);
}

void UFTDamageTextSubsystem::UnregisterDamageMessageListeners()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);

	if (CharacterDamagedListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(CharacterDamagedListenerHandle);
		CharacterDamagedListenerHandle = FGameplayMessageListenerHandle();
	}

	if (DamageTextListenerHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(DamageTextListenerHandle);
		DamageTextListenerHandle = FGameplayMessageListenerHandle();
	}
}

void UFTDamageTextSubsystem::HandleCharacterDamaged(FGameplayTag Channel, const FFTCharacterDamagePayloadStruct& Payload)
{
	if (Payload.DamageAmount <= 0.0f || !ShouldShowDamageTextForLocalPlayer(Payload))
	{
		return;
	}

	ShowDamageText(Payload.DamageAmount, Payload.HitLocation);
}

bool UFTDamageTextSubsystem::ShouldShowDamageTextForLocalPlayer(const FFTCharacterDamagePayloadStruct& Payload) const
{
	return ShouldShowDamageTextForLocalPlayer(Payload.InstigatorActor, Payload.TargetActor);
}

void UFTDamageTextSubsystem::HandleDamageTextMessage(FGameplayTag Channel, const FFTDamageTextPayloadStruct& Payload)
{
	if (Payload.Damage <= 0.0f || !ShouldShowDamageTextForLocalPlayer(Payload.InstigatorActor, Payload.TargetActor))
	{
		return;
	}

	ShowDamageText(Payload.Damage, Payload.HitLocation);
}

bool UFTDamageTextSubsystem::ShouldShowDamageTextForLocalPlayer(AActor* InstigatorActor, AActor* TargetActor) const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	const APawn* LocalPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!LocalPawn)
	{
		return false;
	}

	return TargetActor == LocalPawn || InstigatorActor == LocalPawn;
}

void UFTDamageTextSubsystem::HideDamageText(int32 PoolIndex)
{
	if (!DamageTextPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FFTDamageTextPoolItemStruct& PoolItem = DamageTextPool[PoolIndex];

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PoolItem.HideTimerHandle);
	}

	PoolItem.bInUse = false;
	PoolItem.WorldLocation = FVector::ZeroVector;

	if (PoolItem.Widget)
	{
		PoolItem.Widget->StopAllAnimations();
		PoolItem.Widget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTDamageTextSubsystem::HideAllDamageTexts()
{
	if (UWorld* World = GetWorld())
	{
		for (FFTDamageTextPoolItemStruct& PoolItem : DamageTextPool)
		{
			World->GetTimerManager().ClearTimer(PoolItem.HideTimerHandle);

			PoolItem.bInUse = false;
			PoolItem.WorldLocation = FVector::ZeroVector;

			if (PoolItem.Widget)
			{
				PoolItem.Widget->StopAllAnimations();
				PoolItem.Widget->RemoveFromParent();
				PoolItem.Widget = nullptr;
			}
		}
	}

	DamageTextPool.Reset();
}

UWidgetAnimation* UFTDamageTextSubsystem::FindWidgetAnimation(UUserWidget* Widget, FName AnimationName) const
{
	if (!Widget)
	{
		return nullptr;
	}

	UClass* WidgetClass = Widget->GetClass();
	if (!WidgetClass)
	{
		return nullptr;
	}

	for (TFieldIterator<FObjectProperty> It(WidgetClass); It; ++It)
	{
		FObjectProperty* Property = *It;
		if (!Property)
		{
			continue;
		}

		if (Property->PropertyClass == UWidgetAnimation::StaticClass() &&
			Property->GetFName() == AnimationName)
		{
			return Cast<UWidgetAnimation>(
				Property->GetObjectPropertyValue_InContainer(Widget)
			);
		}
	}

	return nullptr;
}

APlayerController* UFTDamageTextSubsystem::GetOwningPlayerController() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
}

