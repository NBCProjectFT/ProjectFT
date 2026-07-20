#include "FTEscapedRaidWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"

void UFTEscapedRaidWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyResultText();
	BindReturnToBaseButton();
}

void UFTEscapedRaidWidget::NativeDestruct()
{
	UnbindReturnToBaseButton();

	Super::NativeDestruct();
}

void UFTEscapedRaidWidget::SetSettlementText(const FText& NewSettlementText)
{
	if (SettlementText)
	{
		SettlementText->SetText(NewSettlementText);
	}
}

void UFTEscapedRaidWidget::SetRaidResult(EFTRaidResultType NewResultType)
{
	ResultType = NewResultType;
	ApplyResultText();
}

void UFTEscapedRaidWidget::RequestReturnToBase()
{
	FFTMessagePayloadStruct Payload;
	Payload.InstigatorActor = GetOwningPlayerPawn();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Request_Flow_ReturnToBase, Payload);

	UE_LOG(LogFTUI, Log, TEXT("Escaped raid return-to-base message sent. Widget=%s Instigator=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Payload.InstigatorActor));
}

void UFTEscapedRaidWidget::HandleReturnToBaseClicked()
{
	RequestReturnToBase();
}

UButton* UFTEscapedRaidWidget::ResolveReturnToBaseButton() const
{
	if (CachedReturnToBaseButton)
	{
		return CachedReturnToBaseButton;
	}

	if (!WidgetTree)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Escaped raid return-to-base button resolve failed because WidgetTree is missing. Widget=%s"),
			*GetNameSafe(this));
		return nullptr;
	}

	static const FName ReturnToBaseButtonNames[] =
	{
		TEXT("WBP_ReturnToBaseButton_Escaped"),
		TEXT("WBP_ReturnToBaseButton_Escape"),
		TEXT("WBP_ReturnToBaseButton_Fail"),
		TEXT("WBP_ReturnToBaseButton")
	};

	for (const FName ButtonWidgetName : ReturnToBaseButtonNames)
	{
		if (UUserWidget* ButtonWidget = Cast<UUserWidget>(WidgetTree->FindWidget(ButtonWidgetName)))
		{
			return ResolveButtonInsideWidget(ButtonWidget, TEXT("FTGameButton"));
		}
	}

	UE_LOG(LogFTUI, Warning, TEXT("Escaped raid return-to-base button widget was not found. Expected widget name=WBP_ReturnToBaseButton_Escaped Widget=%s"),
		*GetNameSafe(this));
	return nullptr;
}

UButton* UFTEscapedRaidWidget::ResolveButtonInsideWidget(UUserWidget* UserWidget, FName ButtonName) const
{
	if (!UserWidget)
	{
		return nullptr;
	}

	UButton* Button = Cast<UButton>(UserWidget->GetWidgetFromName(ButtonName));
	if (!Button)
	{
		UE_LOG(LogFTUI, Warning, TEXT("Escaped raid inner button was not found. ButtonWidget=%s ExpectedButtonName=%s"),
			*GetNameSafe(UserWidget),
			*ButtonName.ToString());
	}

	return Button;
}

void UFTEscapedRaidWidget::BindReturnToBaseButton()
{
	UButton* ResolvedReturnToBaseButton = ResolveReturnToBaseButton();
	if (!ResolvedReturnToBaseButton)
	{
		return;
	}

	CachedReturnToBaseButton = ResolvedReturnToBaseButton;

	CachedReturnToBaseButton->OnClicked.RemoveAll(this);
	CachedReturnToBaseButton->OnClicked.AddDynamic(this, &ThisClass::HandleReturnToBaseClicked);

	UE_LOG(LogFTUI, Log, TEXT("Escaped raid return-to-base button bound. Widget=%s Button=%s"),
		*GetNameSafe(this),
		*GetNameSafe(CachedReturnToBaseButton));
}

void UFTEscapedRaidWidget::UnbindReturnToBaseButton()
{
	if (CachedReturnToBaseButton)
	{
		CachedReturnToBaseButton->OnClicked.RemoveAll(this);
	}
}

void UFTEscapedRaidWidget::ApplyResultText()
{
	UTextBlock* ResultTextWidget = EscapedText ? EscapedText.Get() : SettlementText.Get();
	if (!ResultTextWidget)
	{
		return;
	}

	const FText& ResultText = ResultType == EFTRaidResultType::Escaped
		? EscapedResultText
		: FailedResultText;
	ResultTextWidget->SetText(ResultText);
}
