#include "FTInventoryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

void UFTInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IMG_PaperBackground)
	{
		PaperMID = IMG_PaperBackground->GetDynamicMaterial();
		StopPaperFlutter();
	}
}

void UFTInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateCornerCurl(InDeltaTime);
}

void UFTInventoryWidget::SetPaperMaterial(UMaterialInterface* InPaperMaterial)
{
	if (!IMG_PaperBackground || !InPaperMaterial)
	{
		return;
	}

	IMG_PaperBackground->SetBrushFromMaterial(InPaperMaterial);
	PaperMID = IMG_PaperBackground->GetDynamicMaterial();
	StopPaperFlutter();
}

void UFTInventoryWidget::PlayOpenPaper()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (PaperMID)
	{
		PaperMID->SetScalarParameterValue(TEXT("FlutterStrength"), 1.0f);
	}

	StartCornerCurl();

	if (UWidgetAnimation* OpenPaperAnimation = FindWidgetAnimation(TEXT("OpenPaper")))
	{
		PlayAnimation(OpenPaperAnimation, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlutterStopTimerHandle);
		World->GetTimerManager().SetTimer(
			FlutterStopTimerHandle,
			this,
			&ThisClass::StopPaperFlutter,
			FlutterDuration,
			false);
	}
}

void UFTInventoryWidget::RefreshItemList()
{
}

void UFTInventoryWidget::UpdateWeight(float CurrentWeight, float MaxWeight)
{
}

void UFTInventoryWidget::ShowItemDetail(FName ItemId)
{
}

void UFTInventoryWidget::StartCornerCurl()
{
	CornerCurlElapsed = 0.0f;
	bUpdatingCornerCurl = true;

	if (PaperMID)
	{
		PaperMID->SetScalarParameterValue(TEXT("FoldProgress"), 0.0f);
		PaperMID->SetScalarParameterValue(TEXT("CornerCurlProgress"), 0.0f);
		PaperMID->SetScalarParameterValue(TEXT("CornerCurlStrength"), 1.0f);
	}
}

void UFTInventoryWidget::UpdateCornerCurl(float DeltaTime)
{
	if (!bUpdatingCornerCurl)
	{
		return;
	}

	CornerCurlElapsed += DeltaTime;
	const float CurlAlpha = FMath::Clamp(CornerCurlElapsed / CornerCurlDuration, 0.0f, 1.0f);
	const float SmoothedCurlAlpha = FMath::InterpEaseOut(0.0f, 1.0f, CurlAlpha, 2.0f);

	if (PaperMID)
	{
		PaperMID->SetScalarParameterValue(TEXT("FoldProgress"), SmoothedCurlAlpha);
		PaperMID->SetScalarParameterValue(TEXT("CornerCurlProgress"), SmoothedCurlAlpha);
	}

	if (CurlAlpha >= 1.0f)
	{
		bUpdatingCornerCurl = false;
	}
}

void UFTInventoryWidget::StopPaperFlutter()
{
	if (PaperMID)
	{
		PaperMID->SetScalarParameterValue(TEXT("FlutterStrength"), 0.0f);
		PaperMID->SetScalarParameterValue(TEXT("FoldProgress"), 1.0f);
		PaperMID->SetScalarParameterValue(TEXT("CornerCurlProgress"), 1.0f);
	}
}

UWidgetAnimation* UFTInventoryWidget::FindWidgetAnimation(FName AnimationName) const
{
	UClass* WidgetClass = GetClass();
	for (TFieldIterator<FObjectProperty> It(WidgetClass); It; ++It)
	{
		FObjectProperty* Property = *It;
		if (Property->PropertyClass == UWidgetAnimation::StaticClass() && Property->GetFName() == AnimationName)
		{
			return Cast<UWidgetAnimation>(Property->GetObjectPropertyValue_InContainer(this));
		}
	}

	return nullptr;
}
