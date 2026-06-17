#include "FTHUD.h"

#include "Blueprint/UserWidget.h"

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
