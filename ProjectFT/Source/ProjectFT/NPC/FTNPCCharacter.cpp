#include "FTNPCCharacter.h"

#include "Components/WidgetComponent.h"
#include "ProjectFT/NPC/FTNPCAIController.h"
#include "ProjectFT/UI/FTNPCReportGaugeWidget.h"

AFTNPCCharacter::AFTNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	ReportGaugeWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ReportGaugeWidgetComponent"));
	ReportGaugeWidgetComponent->SetupAttachment(RootComponent);
	ReportGaugeWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ReportGaugeWidgetComponent->SetDrawSize(FVector2D(140.0f, 16.0f));
	ReportGaugeWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
}

void AFTNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UFTNPCReportGaugeWidget* ReportGaugeWidget = Cast<UFTNPCReportGaugeWidget>(ReportGaugeWidgetComponent->GetWidget()))
	{
		ReportGaugeWidget->SetReportOwnerActor(this);
	}
}

void AFTNPCCharacter::OnImmobilizedStateChanged(bool bImmobilized)
{
	Super::OnImmobilizedStateChanged(bImmobilized);

	if (AFTNPCAIController* NPCAIController = Cast<AFTNPCAIController>(GetController()))
	{
		NPCAIController->HandleStunStateChanged(bImmobilized);
	}
}

void AFTNPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFTNPCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
