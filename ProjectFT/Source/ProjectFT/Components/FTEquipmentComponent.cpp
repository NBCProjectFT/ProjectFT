// Fill out your copyright notice in the Description page of Project Settings.

#include "FTEquipmentComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_ItemAbility.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTMeleeDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"

UFTEquipmentComponent::UFTEquipmentComponent()
{
	// 테스트용 컴포넌트이므로 Tick을 켭니다.
	// 현재 Tick은 임시 공격 입력을 확인하는 용도로 사용됩니다.
	// 정식 구조에서는 입력 처리를 Enhanced Input으로 옮기면 Tick을 끌 수 있습니다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// 기본 아이템 액터 클래스입니다.
	// BP에서 다른 AFTItemActor 파생 클래스로 교체할 수 있습니다.
	ItemActorClass = AFTItemActor::StaticClass();
}

void UFTEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay 때 기본 아이템을 자동 장착합니다.
	//
	// 조건:
	// - bEquipDefaultOnBeginPlay == true
	// - DefaultItemData가 지정되어 있음
	//
	// 목적:
	// - 캐릭터 BP에 이 컴포넌트를 붙이고 DefaultItemData만 넣으면
	//   플레이 시작과 동시에 테스트용 아이템이 손에 붙게 하기 위함입니다.
	if (bEquipDefaultOnBeginPlay && DefaultItemData)
	{
		EquipItem(DefaultItemData);
	}

	// BeginPlay 때 기본 아이템의 UseAbility를 ASC에 미리 부여합니다.
	//
	// 조건:
	// - bGrantDefaultItemAbilityOnBeginPlay == true
	// - DefaultItemData가 지정되어 있음
	//
	// 주의:
	// - TryAttack()에서도 EnsureAbilityGranted()를 다시 호출합니다.
	// - 따라서 여기서 미리 부여하지 않아도 공격 시점에 부여될 수 있습니다.
	if (bGrantDefaultItemAbilityOnBeginPlay && DefaultItemData)
	{
		EnsureAbilityGranted(ResolveOwnerAbilitySystem(), DefaultItemData);
	}
}

void UFTEquipmentComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 임시 공격 입력이 꺼져 있거나,
	// 이 컴포넌트의 Owner가 로컬 플레이어가 아니라면 입력을 처리하지 않습니다.
	//
	// 이유:
	// - WasInputKeyJustPressed는 로컬 PlayerController 기준 입력 확인입니다.
	// - AI나 원격 플레이어에서 처리하면 의도치 않은 동작이 생길 수 있습니다.
	if (!bEnableTemporaryAttackInput || !IsLocalPlayerOwner())
	{
		return;
	}

	// Owner가 Character라면 Controller를 가져옵니다.
	// 이 Controller가 PlayerController일 때만 키 입력 확인이 가능합니다.
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	const APlayerController* PlayerController =
		OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;

	// TemporaryAttackKey가 이번 프레임에 눌렸다면 공격을 시도합니다.
	//
	// 현재 기본값:
	// - LeftMouseButton
	//
	// 정식 구조에서는 이 부분을 Enhanced Input의 InputAction 바인딩으로 교체하는 것이 좋습니다.
	if (PlayerController && PlayerController->WasInputKeyJustPressed(TemporaryAttackKey))
	{
		TryAttack();
	}
}

void UFTEquipmentComponent::EquipItem(UFTItemDataAsset* ItemData)
{
	// ItemData가 없으면 장착할 아이템이 없다는 뜻으로 보고,
	// 현재 장착 아이템을 해제합니다.
	if (!ItemData)
	{
		UnequipCurrentItem();
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	USkeletalMeshComponent* OwnerMesh = ResolveOwnerMesh();

	// 장착에 필요한 기본 조건을 검사합니다.
	//
	// 필요한 것:
	// - Owner: 이 컴포넌트를 가진 액터
	// - World: SpawnActor를 호출할 월드
	// - OwnerMesh: 아이템을 붙일 SkeletalMeshComponent
	// - ItemActorClass: Spawn할 아이템 액터 클래스
	if (!Owner || !World || !OwnerMesh || !ItemActorClass)
	{
		UE_LOG(LogFTItem, Warning, TEXT("EquipItem failed. Owner=%s World=%s Mesh=%s ItemActorClass=%s"),
			*GetNameSafe(Owner),
			World ? TEXT("Valid") : TEXT("Missing"),
			*GetNameSafe(OwnerMesh),
			*GetNameSafe(ItemActorClass));

		return;
	}

	// 새 아이템을 장착하기 전에 기존 장착 아이템을 제거합니다.
	// 한 번에 하나의 아이템만 장착하는 테스트 구조입니다.
	UnequipCurrentItem();

	// 아이템 액터 Spawn 설정입니다.
	FActorSpawnParameters SpawnParams;

	// Owner를 현재 컴포넌트의 Owner로 지정합니다.
	// 나중에 데미지 처리나 소유자 확인에 사용할 수 있습니다.
	SpawnParams.Owner = Owner;

	// Instigator는 보통 Pawn 기준 공격자 정보를 의미합니다.
	// Owner->GetInstigator()를 그대로 넘깁니다.
	SpawnParams.Instigator = Owner->GetInstigator();

	// 장착용 아이템은 충돌 때문에 Spawn 실패하면 안 되므로 AlwaysSpawn을 사용합니다.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 실제 아이템 액터를 월드에 생성합니다.
	EquippedItemActor = World->SpawnActor<AFTItemActor>(
		ItemActorClass,
		Owner->GetActorTransform(),
		SpawnParams
	);

	if (!EquippedItemActor)
	{
		return;
	}

	// 생성된 아이템 액터에 데이터 에셋을 연결합니다.
	// AFTItemActor는 이 ItemData를 기준으로 외형/기능 데이터를 사용할 수 있습니다.
	EquippedItemActor->ItemData = ItemData;

	// ItemData를 기준으로 아이템 외형을 갱신합니다.
	// 예:
	// - StaticMesh 변경
	// - SkeletalMesh 변경
	// - Material 변경
	EquippedItemActor->UpdateAppearance();

	// 장착된 아이템은 캐릭터 손에 붙어 있는 장식/무기이므로
	// 자체 충돌은 끕니다.
	EquippedItemActor->SetActorEnableCollision(false);

	// 액터 전체 충돌만 끄는 것으로 부족할 수 있으므로,
	// 내부 PrimitiveComponent들도 직접 순회하면서 충돌/물리/오버랩을 끕니다.
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	EquippedItemActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		// 장착 중에는 물리 충돌을 사용하지 않습니다.
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// 장착된 아이템이 물리 시뮬레이션으로 떨어지거나 흔들리지 않게 합니다.
		PrimitiveComponent->SetSimulatePhysics(false);

		// 장착 상태에서는 오버랩 이벤트도 끕니다.
		// 공격 판정은 나중에 Ability/Trace에서 별도로 처리하는 것이 좋습니다.
		PrimitiveComponent->SetGenerateOverlapEvents(false);
	}

	// ItemData에서 장착 소켓 이름을 결정합니다.
	//
	// 현재 ResolveAttachSocketName() 로직:
	// - UFTMeleeDataAsset이면 MeleeActionData.AttachSocketName 사용
	// - 없으면 FallbackAttachSocketName 사용
	const FName AttachSocketName = ResolveAttachSocketName(ItemData);

	// 소켓 이름이 비어 있지 않은데 OwnerMesh에 해당 소켓이 없다면 경고를 출력합니다.
	// 그래도 장착은 실패시키지 않고 Mesh Root에 붙입니다.
	if (!AttachSocketName.IsNone() && !OwnerMesh->DoesSocketExist(AttachSocketName))
	{
		UE_LOG(LogFTItem, Warning, TEXT("Equip socket '%s' does not exist on mesh '%s'. Item will attach to mesh root."),
			*AttachSocketName.ToString(),
			*GetNameSafe(OwnerMesh));
	}

	// 아이템 액터를 캐릭터 Mesh에 Attach합니다.
	//
	// FAttachmentTransformRules::SnapToTargetNotIncludingScale:
	// - 위치/회전은 소켓에 맞춤
	// - 스케일은 부모 스케일을 무조건 따라가지 않게 함
	//
	// 소켓이 실제로 존재하면 해당 소켓에 붙이고,
	// 존재하지 않으면 NAME_None으로 Mesh Root에 붙입니다.
	EquippedItemActor->AttachToComponent(
		OwnerMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		OwnerMesh->DoesSocketExist(AttachSocketName) ? AttachSocketName : NAME_None
	);

	// 소켓에 붙인 뒤 상대 Transform 보정값을 적용합니다.
	//
	// 손에 붙었지만 위치/회전/크기가 어긋나는 경우
	// BP에서 이 값들을 조절해서 맞출 수 있습니다.
	EquippedItemActor->SetActorRelativeLocation(RelativeLocation);
	EquippedItemActor->SetActorRelativeRotation(RelativeRotation);
	EquippedItemActor->SetActorRelativeScale3D(RelativeScale);

	UE_LOG(LogFTItem, Log, TEXT("Equipped item. Owner=%s Item=%s Socket=%s Actor=%s"),
		*GetNameSafe(Owner),
		*GetNameSafe(ItemData),
		*AttachSocketName.ToString(),
		*GetNameSafe(EquippedItemActor));
}

void UFTEquipmentComponent::UnequipCurrentItem()
{
	// 현재 장착된 아이템 액터가 있으면 제거합니다.
	//
	// 테스트용 구조에서는 장착 해제 시 그냥 Destroy합니다.
	// 정식 구조에서는 바닥에 떨어뜨리기, 인벤토리로 되돌리기 등을 추가할 수 있습니다.
	if (EquippedItemActor)
	{
		EquippedItemActor->Destroy();
		EquippedItemActor = nullptr;
	}
}

bool UFTEquipmentComponent::TryAttack()
{
	// 현재 테스트 구조에서는 "현재 장착된 아이템"이 아니라
	// DefaultItemData를 기준으로 공격합니다.
	//
	// 나중에 정식 장착 시스템이 생기면
	// EquippedItemActor->ItemData 또는 CurrentEquippedItemData를 사용하도록 바꾸는 것이 좋습니다.
	UFTItemDataAsset* ItemData = DefaultItemData;

	if (!ItemData)
	{
		UE_LOG(LogFTItem, Warning, TEXT("Temporary attack failed: DefaultItemData is missing. Owner=%s"),
			*GetNameSafe(GetOwner()));

		return false;
	}

	// ItemData 안의 UseData에서 사용할 Ability 클래스를 가져옵니다.
	//
	// 예:
	// - GA_MeleeAction
	// - GA_HitscanFire
	// - GA_ProjectileFire
	const TSubclassOf<UFTGameplayAbility> UseAbility = ItemData->ItemData.UseData.UseAbility;

	if (!UseAbility)
	{
		UE_LOG(LogFTItem, Warning, TEXT("Temporary attack failed: UseAbility is missing. Item=%s"),
			*GetNameSafe(ItemData));

		return false;
	}

	// Owner의 AbilitySystemComponent를 찾습니다.
	//
	// Ability 실행은 ASC를 통해 이루어집니다.
	UAbilitySystemComponent* AbilitySystemComponent = ResolveOwnerAbilitySystem();

	if (!AbilitySystemComponent)
	{
		UE_LOG(LogFTItem, Warning, TEXT("Temporary attack failed: ASC is missing. Owner=%s"),
			*GetNameSafe(GetOwner()));

		return false;
	}

	// 공격에 사용할 Ability가 ASC에 부여되어 있는지 확인합니다.
	// 아직 없으면 GiveAbility로 부여합니다.
	EnsureAbilityGranted(AbilitySystemComponent, ItemData);

	// Ability CDO에서 TriggerEventTag를 가져옵니다.
	//
	// 이 프로젝트 구조에서는 Ability를 직접 TryActivateAbility로 실행하지 않고,
	// GameplayEvent로 실행하는 흐름을 사용합니다.
	//
	// 즉:
	// ASC->HandleGameplayEvent(EventTag, Payload)
	// ↓
	// 해당 EventTag를 AbilityTriggers에 등록한 Ability가 실행됨
	const UFTGameplayAbility* AbilityCDO = UseAbility.GetDefaultObject();
	const FGameplayTag EventTag = AbilityCDO ? AbilityCDO->GetTriggerEventTag() : FGameplayTag();

	if (!EventTag.IsValid())
	{
		UE_LOG(LogFTItem, Warning, TEXT("Temporary attack failed: Ability has no trigger tag. Ability=%s Item=%s"),
			*GetNameSafe(AbilityCDO),
			*GetNameSafe(ItemData));

		return false;
	}

	const FTItemUseStruct& UseData = ItemData->ItemData.UseData;

	// 아이템 UseData에 CooldownSeconds가 설정되어 있고,
	// 현재 ASC에 해당 쿨다운 태그가 붙어 있다면 공격을 막습니다.
	//
	// 쿨다운 태그는 UFTGA_ItemAbility::ResolveCooldownTag(UseData)로 계산합니다.
	//
	// 의미:
	// - 이미 이 아이템/능력의 쿨다운 중이면
	//   GameplayEvent를 보내지 않고 false를 반환합니다.
	if (UseData.CooldownSeconds > 0.0f
		&& AbilitySystemComponent->HasMatchingGameplayTag(UFTGA_ItemAbility::ResolveCooldownTag(UseData)))
	{
		UE_LOG(LogFTItem, Verbose, TEXT("Temporary attack blocked by cooldown. Owner=%s Item=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(ItemData));

		return false;
	}

	// Ability에 전달할 GameplayEvent Payload를 구성합니다.
	FGameplayEventData Payload;

	// 어떤 Event인지 표시합니다.
	Payload.EventTag = EventTag;

	// 이 이벤트를 발생시킨 주체입니다.
	// 현재는 컴포넌트 Owner를 사용합니다.
	Payload.Instigator = GetOwner();

	// 이벤트의 대상입니다.
	// 현재는 자기 자신 기준으로 공격 Ability를 실행하므로 Owner를 사용합니다.
	Payload.Target = GetOwner();

	// Ability에 넘길 추가 데이터입니다.
	// 여기서는 ItemData를 넘겨서 Ability가 어떤 아이템으로 실행되었는지 알 수 있게 합니다.
	//
	// Ability 쪽에서는 TriggerEventData->OptionalObject를 UFTItemDataAsset으로 Cast해서 사용할 수 있습니다.
	Payload.OptionalObject = ItemData;

	// GameplayEvent를 ASC에 전달합니다.
	//
	// 이 호출이 성공하면,
	// EventTag를 Trigger로 등록한 Ability가 활성화됩니다.
	//
	// 반환값:
	// - 이 Event로 활성화된 Ability 개수
	const int32 ActivatedCount = AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);

	UE_LOG(LogFTItem, Log, TEXT("Temporary attack requested. Owner=%s Item=%s EventTag=%s ActivatedCount=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(ItemData),
		*EventTag.ToString(),
		ActivatedCount);

	return ActivatedCount > 0;
}

FName UFTEquipmentComponent::ResolveAttachSocketName(const UFTItemDataAsset* ItemData) const
{
	// ItemData가 근접 아이템 데이터라면,
	// 근접 데이터 안에 있는 장착 소켓 이름을 우선 사용합니다.
	//
	// 현재 구조:
	// UFTMeleeDataAsset
	// └─ MeleeActionData
	//    └─ AttachSocketName
	if (const UFTMeleeDataAsset* MeleeDataAsset = Cast<UFTMeleeDataAsset>(ItemData))
	{
		if (!MeleeDataAsset->MeleeActionData.AttachSocketName.IsNone())
		{
			return MeleeDataAsset->MeleeActionData.AttachSocketName;
		}
	}

	// 근접 데이터가 아니거나,
	// AttachSocketName이 비어 있으면 기본 소켓 이름을 사용합니다.
	return FallbackAttachSocketName;
}

USkeletalMeshComponent* UFTEquipmentComponent::ResolveOwnerMesh() const
{
	// Owner가 Character라면 Character의 기본 Mesh를 사용합니다.
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		return Character->GetMesh();
	}

	// Character가 아니라면 Owner 안에서 SkeletalMeshComponent를 검색합니다.
	//
	// 예:
	// - 테스트용 Actor
	// - 커스텀 Pawn
	// - SkeletalMeshComponent를 가진 임의 액터
	return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

UAbilitySystemComponent* UFTEquipmentComponent::ResolveOwnerAbilitySystem() const
{
	// Owner에서 AbilitySystemComponent를 찾습니다.
	//
	// UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent()는
	// Owner가 IAbilitySystemInterface를 구현했거나,
	// ASC를 적절히 찾을 수 있는 구조일 때 동작합니다.
	return GetOwner() ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()) : nullptr;
}

bool UFTEquipmentComponent::EnsureAbilityGranted(
	UAbilitySystemComponent* AbilitySystemComponent,
	UFTItemDataAsset* ItemData) const
{
	// ASC, ItemData, UseAbility 중 하나라도 없으면 Ability를 부여할 수 없습니다.
	if (!AbilitySystemComponent || !ItemData || !ItemData->ItemData.UseData.UseAbility)
	{
		return false;
	}

	const TSubclassOf<UFTGameplayAbility> UseAbility = ItemData->ItemData.UseData.UseAbility;

	// 이미 같은 Ability 클래스가 ASC에 부여되어 있는지 검사합니다.
	//
	// GetActivatableAbilities():
	// - 현재 ASC가 가지고 있는 Ability 목록을 반환합니다.
	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.Ability->GetClass() == UseAbility)
		{
			// 이미 부여되어 있으므로 다시 GiveAbility 하지 않습니다.
			return true;
		}
	}

	// Ability가 아직 없으면 ASC에 새로 부여합니다.
	//
	// 테스트용 구조:
	// - 아이템을 사용하려는 순간 Ability가 없으면 즉석에서 부여합니다.
	//
	// 주의:
	// - 멀티플레이에서는 일반적으로 서버 권한에서 GiveAbility를 호출해야 합니다.
	// - 싱글플레이 테스트라면 크게 문제되지 않습니다.
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UseAbility));

	UE_LOG(LogFTItem, Log, TEXT("Temporary attack granted ability. Owner=%s Item=%s Ability=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(ItemData),
		*GetNameSafe(UseAbility));

	return true;
}

bool UFTEquipmentComponent::IsLocalPlayerOwner() const
{
	// Owner가 Character이고,
	// 현재 로컬 플레이어가 조종 중인 Character인지 확인합니다.
	//
	// 임시 입력 처리는 로컬 플레이어에게만 필요하므로 이 검사를 사용합니다.
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	return OwnerCharacter && OwnerCharacter->IsLocallyControlled();
}