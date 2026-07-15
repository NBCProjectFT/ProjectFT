#include "FTItemActor.h"

#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectFT/Message/FTGameplayTags.h"
#include "ProjectFT/Struct/FTMessagePayloadStruct.h"
#include "FTItemPoolSubsystem.h"
#include "Components/WidgetComponent.h"
#include "ProjectFT/UI/FTItemTooltipWidget.h"


AFTItemActor::AFTItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsBody"));

	TooltipWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TooltipWidget"));
	TooltipWidgetComponent->SetupAttachment(RootComponent);
	TooltipWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TooltipWidgetComponent->SetDrawSize(FVector2D(220.0f, 100.0f));
	TooltipWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	TooltipWidgetComponent->SetVisibility(false);

	// WBP_ItemTooltip 위젯 에셋 클래스 자동 바인딩
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/Game/UI/Inventory/WBP_ItemTooltip.WBP_ItemTooltip_C"));
	if (WidgetClassFinder.Succeeded())
	{
		TooltipWidgetComponent->SetWidgetClass(WidgetClassFinder.Class);
	}
}

void AFTItemActor::BeginPlay()
{
	Super::BeginPlay();

	// 에디터 디테일 패널에서 수정한 크기를 런타임에 최종 반영합니다.
	if (TooltipWidgetComponent)
	{
		TooltipWidgetComponent->SetDrawSize(TooltipDrawSize);
	}

	UpdateAppearance();

	if (UWorld* World = GetWorld())
	{
		if (UFTItemPoolSubsystem* PoolSubsystem = World->GetSubsystem<UFTItemPoolSubsystem>())
		{
			PoolSubsystem->RegisterActiveItem(this);
		}
	}
}

bool AFTItemActor::Interact_Implementation(AActor* Interactor)
{
	if (!ItemData) return false;

	// 인벤토리 무게 한도 등으로 추가할 수 있는지 선검증
	UFTInventoryComponent* InventoryComp = Interactor->FindComponentByClass<UFTInventoryComponent>();
	if (InventoryComp)
	{
		if (!InventoryComp->CanAddItem(ItemData->ItemData.ItemId, 1))
		{
			UE_LOG(LogFTItem, Warning, TEXT("%s 획득 실패: 인벤토리 무게 한도 초과"), *ItemData->ItemData.ItemName.ToString());
			return false;
		}
	}
	
	// GameplayMessageSubsystem을 통해 아이템 획득 메시지 전송
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FFTMessagePayloadStruct Payload;
	Payload.ItemId = ItemData->ItemData.ItemId;
	Payload.InstigatorActor = Interactor;
	Payload.TargetActor = this;
	Payload.Value = 1.0f;

	MessageSubsystem.BroadcastMessage(TAG_FT_Event_ItemPickedUp, Payload);
	
	UE_LOG(LogFTItem, Log, TEXT("%s 아이템 획득"), *ItemData->ItemData.ItemName.ToString());
	
	DestroyItem();
	return true;
}

void AFTItemActor::UpdateAppearance()
{
	if (ItemData && ItemData->ItemData.ItemMesh)
	{
		MeshComponent->SetStaticMesh(ItemData->ItemData.ItemMesh);
		MeshComponent->SetRelativeScale3D(ItemData->ItemData.DropMeshScale);
	}
}

void AFTItemActor::DestroyItem()
{
	if (UWorld* World = GetWorld())
	{
		if (UFTItemPoolSubsystem* PoolSubsystem = World->GetSubsystem<UFTItemPoolSubsystem>())
		{
			// 파괴 권한을 서브시스템으로 양도 (풀링 반환 처리)
			PoolSubsystem->ReleaseItemActor(this);
			return;
		}
	}
	
	// 서브시스템이 없거나 오류 상황일 때만 직접 파괴 실행
	Destroy();
}

void AFTItemActor::SetTooltipVisibility(bool bVisible)
{
	if (!TooltipWidgetComponent)
	{
		UE_LOG(LogFTItem, Warning, TEXT("SetTooltipVisibility 실패: [%s]의 TooltipWidgetComponent가 nullptr입니다."), *GetName());
		return;
	}
	
	if (bVisible)
	{
		// 플레이어와의 거리에 비례해서 Z 높이를 보정 (가까워질수록 툴팁이 낮아져 화면 위로 잘림 방지)
		float Distance = 200.0f;
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					Distance = FVector::Dist(Pawn->GetActorLocation(), GetActorLocation());
				}
			}
		}

		// 거리가 가까울수록 Z 높이를 점진적으로 낮춤 (최소 45cm ~ 최대 에디터 지정값)
		const float MaxDist = 450.0f;
		const float Alpha = FMath::Clamp(Distance / MaxDist, 0.0f, 1.0f);
		const float DynamicZ = FMath::Lerp(45.0f, TooltipRelativeLocation.Z, Alpha);

		FVector TargetWorldLocation = GetActorLocation() + FVector(0.0f, 0.0f, DynamicZ);
		TooltipWidgetComponent->SetWorldLocation(TargetWorldLocation);

		// 렌더 타겟 재생성으로 인한 깜빡임(보였다 안보였다 함)을 방지하기 위해 
		// DrawSize 설정값이 다를 때만 최초 1회 업데이트하도록 변경합니다.
		if (TooltipWidgetComponent->GetDrawSize() != FIntPoint(TooltipDrawSize.X, TooltipDrawSize.Y))
		{
			TooltipWidgetComponent->SetDrawSize(TooltipDrawSize);
		}

		// 위젯 인스턴스가 아직 초기화되지 않았다면 강제로 초기화
		if (!TooltipWidgetComponent->GetUserWidgetObject())
		{
			TooltipWidgetComponent->InitWidget();
		}

		if (!TooltipWidgetComponent->IsVisible())
		{
			TooltipWidgetComponent->SetVisibility(true);
		}

		UUserWidget* UserWidget = TooltipWidgetComponent->GetUserWidgetObject();
		if (!UserWidget)
		{
			return;
		}

		UFTItemTooltipWidget* TooltipWidget = Cast<UFTItemTooltipWidget>(UserWidget);
		if (TooltipWidget)
		{
			if (ItemData)
			{
				TooltipWidget->SetItemInfo(ItemData->ItemData.ItemName, ItemData->ItemData.Weight, ItemData->ItemData.Cost);
			}
		}
	}
	else
	{
		if (TooltipWidgetComponent->IsVisible())
		{
			TooltipWidgetComponent->SetVisibility(false);
		}
	}
}