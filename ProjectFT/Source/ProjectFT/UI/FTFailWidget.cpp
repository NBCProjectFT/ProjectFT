#include "FTFailWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

void UFTFailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindReturnToBaseButton();
}

void UFTFailWidget::NativeDestruct()
{
	UnbindReturnToBaseButton();

	Super::NativeDestruct();
}

void UFTFailWidget::ShowFailReason(const FText& FailReason)
{
}

void UFTFailWidget::ShowLostItemInfo(const TArray<FName>& LostItemIds)
{
}

void UFTFailWidget::RequestRetry()
{
}

void UFTFailWidget::RequestReturnToBase()
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetOwningPlayerPawn();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Request_Flow_ReturnToBase, Payload);

	UE_LOG(LogFTUI, Log, TEXT("Fail return-to-base message sent. Widget=%s Instigator=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Payload.InstigatorActor));
}

void UFTFailWidget::HandleReturnToBaseClicked()
{
	RequestReturnToBase();
}

UButton* UFTFailWidget::ResolveReturnToBaseButton() const
{
	if (CachedReturnToBaseButton)
	{
		return CachedReturnToBaseButton;
	}

	if (!WidgetTree)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Fail return-to-base button resolve failed because WidgetTree is missing. Widget=%s"),
			*GetNameSafe(this));
		return nullptr;
	}

	if (UUserWidget* ButtonWidget = Cast<UUserWidget>(WidgetTree->FindWidget(TEXT("WBP_ReturnToBaseButton_Fail"))))
	{
		return ResolveButtonInsideWidget(ButtonWidget, TEXT("FTGameButton"));
	}

	UE_LOG(LogFTUI, Warning, TEXT("Fail return-to-base button widget was not found. Expected widget name=WBP_ReturnToBaseButton_Fail Widget=%s"),
		*GetNameSafe(this));
	return nullptr;
}

UButton* UFTFailWidget::ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const
{
	if (!UserWidget)
	{
		return nullptr;
	}

	UButton* Button = Cast<UButton>(UserWidget->GetWidgetFromName(ButtonName));
	if (!Button)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Fail inner button was not found. ButtonWidget=%s ExpectedButtonName=%s"),
			*GetNameSafe(UserWidget),
			*ButtonName.ToString());
	}

	return Button;
}

void UFTFailWidget::BindReturnToBaseButton()
{
	UButton* ResolvedReturnToBaseButton = ResolveReturnToBaseButton();
	if (!ResolvedReturnToBaseButton)
	{
		return;
	}

	CachedReturnToBaseButton = ResolvedReturnToBaseButton;

	CachedReturnToBaseButton->OnClicked.RemoveAll(this);
	CachedReturnToBaseButton->OnClicked.AddDynamic(this, &ThisClass::HandleReturnToBaseClicked);

	UE_LOG(LogFTUI, Log, TEXT("Fail return-to-base button bound. Widget=%s Button=%s"),
		*GetNameSafe(this),
		*GetNameSafe(CachedReturnToBaseButton));
}

void UFTFailWidget::UnbindReturnToBaseButton()
{
	if (CachedReturnToBaseButton)
	{
		CachedReturnToBaseButton->OnClicked.RemoveAll(this);
	}
}
