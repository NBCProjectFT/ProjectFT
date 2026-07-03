#include "FTMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

void UFTMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindStartButtonEvents();
}

UButton* UFTMainMenuWidget::ResolveStartButton() const
{
	if (CachedStartButton)
	{
		return CachedStartButton;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	if (UUserWidget* ButtonWidget = Cast<UUserWidget>(WidgetTree->FindWidget(TEXT("WBP_StartButton"))))
	{
		return ResolveButtonInsideWidget(ButtonWidget, TEXT("FTGameButton"));
	}

	return nullptr;
}

UButton* UFTMainMenuWidget::ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const
{
	if (!UserWidget)
	{
		return nullptr;
	}

	return Cast<UButton>(UserWidget->GetWidgetFromName(ButtonName));
}

void UFTMainMenuWidget::BindStartButtonEvents()
{
	UButton* ResolvedStartButton = ResolveStartButton();
	if (!ResolvedStartButton)
	{
		return;
	}

	CachedStartButton = ResolvedStartButton;

	CachedStartButton->OnClicked.RemoveAll(this);
	CachedStartButton->OnClicked.AddDynamic(this, &UFTMainMenuWidget::HandleStartButtonClicked);
}

void UFTMainMenuWidget::HandleStartButtonClicked()
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetOwningPlayerPawn();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Request_Flow_StartGame, Payload);
}
