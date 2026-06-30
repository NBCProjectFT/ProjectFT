#include "FTHUD.h"

#include "FTMainHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"

void AFTHUD::CreateMainHUD()
{
}

void AFTHUD::ShowWidget(UUserWidget* Widget)
{
	if (Widget)
	{
		Widget->AddToViewport();
	}
}

void AFTHUD::HideWidget(UUserWidget* Widget)
{
	if (Widget)
	{
		Widget->RemoveFromParent();
	}
}

void AFTHUD::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("OwningPlayerController is null"));
		return;
	}
	
	if (HUDClass)
	{
		if (UFTMainHUDWidget* CreatedMainHUDWidget = Cast<UFTMainHUDWidget>(CreateWidget(PC, HUDClass)))
		{
			MainHUDWidget = CreatedMainHUDWidget;
			MainHUDWidget->AddToViewport();

			if (bEnableHUDDrainTest)
			{
				TestHP = 100.0f;
				TestStamina = 100.0f;
				MainHUDWidget->UpdateHPValue(TestHP, 100.0f);
				MainHUDWidget->UpdateStamina(TestStamina, 100.f);
				GetWorldTimerManager().SetTimer(HUDDrainTestTimerHandle, this, &ThisClass::UpdateHUDDrainTest, HUDDrainTestInterval, true);
			}
		}
	}
}

void AFTHUD::UpdateHUDDrainTest()
{
	if (!MainHUDWidget)
	{
		GetWorldTimerManager().ClearTimer(HUDDrainTestTimerHandle);
		return;
	}

	TestHP = FMath::Max(TestHP - HUDDrainTestAmount, 0.0f);
	TestStamina = FMath::Max(TestStamina - HUDDrainTestAmount, 0.0f);

	MainHUDWidget->UpdateHPValue(TestHP, 100.0f);
	MainHUDWidget->UpdateStamina(TestStamina, 100.f);

	if (TestHP <= 0.0f && TestStamina <= 0.0f)
	{
		GetWorldTimerManager().ClearTimer(HUDDrainTestTimerHandle);
	}
}
