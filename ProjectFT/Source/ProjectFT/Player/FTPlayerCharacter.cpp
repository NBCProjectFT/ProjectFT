// Fill out your copyright notice in the Description page of Project Settings.

#include "FTPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_ItemAbility.h"
#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/AbilitySystem/FTPlayerAttributeSet.h"
#include "ProjectFT/Components/FTInventoryComponent.h"
#include "ProjectFT/Components/FTInteractionComponent.h"
#include "ProjectFT/Components/FTCaptureEscapeComponent.h"
#include "ProjectFT/Components/FTCrosshairComponent.h"
#include "ProjectFT/Components/FTTailComponent.h"
#include "ProjectFT/Components/FTTraversalComponent.h"
#include "ProjectFT/Core/FTLogChannels.h"
#include "ProjectFT/Data/FTItemDataAsset.h"
#include "ProjectFT/Data/FTMeleeDataAsset.h"
#include "ProjectFT/Data/FTHitScanDataAsset.h"
#include "ProjectFT/Data/FTLauncherDataAsset.h"
#include "ProjectFT/Data/FTThrowDataAsset.h"
#include "ProjectFT/Item/FTItemActor.h"
#include "ProjectFT/UI/FTUIManagerSubsystem.h"
#include "ProjectFT/ViewModel/FTInventoryViewModel.h"


// Sets default values
AFTPlayerCharacter::AFTPlayerCharacter()
{
	// 플레이어 캐릭터는 매 프레임 Tick한다(앉기 시점 보간, 스프린트/회복 등).
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	// 조준형(스트레이프) TPS: 본체 yaw를 컨트롤러(카메라) 회전에 맞춘다. 좌우 입력 시 옆으로 스트레이프.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		// MaxWalkSpeed 초기값(프리뷰)은 베이스 ctor가 MoveSpeed 속성에서 셋한다.
		Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
		Movement->MaxWalkSpeedCrouched = 300.0f;
	}

	// 3인칭 카메라 붐: 캡슐 상단(머리 높이) 피벗에서 컨트롤러 회전(pitch/yaw)을 따라 돌고, 뒤로 일정 거리 빠진다.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;   // 붐이 컨트롤러 회전을 따른다.
	CameraBoom->bDoCollisionTest = true;          // 벽에 가리면 카메라를 앞으로 당겨 클리핑 방지.
	// 조준형(스트레이프) TPS용 오버숄더 오프셋. 에디터(BP)에서 조절 가능.
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 0.0f);

	// 붐 끝의 추적 카메라. 회전은 붐이 담당하므로 카메라 자체는 폰 회전을 따르지 않는다.
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	TailComponent = CreateDefaultSubobject<UFTTailComponent>(TEXT("TailComponent"));
	TailComponent->SetupAttachment(GetMesh(), TEXT("Tail"));

	CrosshairComponent = CreateDefaultSubobject<UFTCrosshairComponent>(TEXT("CrosshairComponent"));

	// 상호작용 컴포넌트.
	InteractionComponent = CreateDefaultSubobject<UFTInteractionComponent>(TEXT("InteractionComponent"));

	// 트레이스 기반 파쿠르 컴포넌트(필요 시 MotionWarpingComponent를 런타임에 스스로 추가한다).
	TraversalComponent = CreateDefaultSubobject<UFTTraversalComponent>(TEXT("TraversalComponent"));

	// 붙잡힘(경비 잡기) 상태 + 좌우연타 탈출 게이지.
	CaptureEscapeComponent = CreateDefaultSubobject<UFTCaptureEscapeComponent>(TEXT("CaptureEscapeComponent"));

	// GAS: 플레이어 전용 속성셋만 여기서 생성한다(ASC·공용 AttributeSet은 베이스 AFTCharacterBase가 생성).
	// 캐릭터 서브오브젝트라 베이스의 ASC가 자동 등록한다.
	PlayerAttributeSet = CreateDefaultSubobject<UFTPlayerAttributeSet>(TEXT("PlayerAttributeSet"));

	// 손에 든 아이템 비주얼의 기본 스폰 클래스. 필요하면 BP에서 파생 클래스로 교체한다.
	HeldItemActorClass = AFTItemActor::StaticClass();
}

// Called when the game starts or when spawned
void AFTPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 공용 AttributeSet과 플레이어 전용 AttributeSet이 ASC에 등록되고 ActorInfo가 초기화된 뒤,
	// Player BP/인스턴스에서 지정한 초기 base value를 한 번 적용한다.
	ApplyInitialAbilitySystemAttributes();

	// BP가 네이티브 생성자의 캡슐 크기를 덮어쓴 뒤의 실제 값을 기준으로 앉은 높이를 계산한다.
	InitializeCrouchCapsuleSize();

	if (UFTInventoryComponent* Inventory = GetInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddDynamic(this, &AFTPlayerCharacter::OnInventoryChangedCallback);
	}

	if (InteractionComponent)
	{
		InteractionComponent->OnActiveChannelChanged.AddDynamic(this, &AFTPlayerCharacter::HandleActiveChannelChanged);
	}

	if (AbilitySystemComponent)
	{
		// InitAbilityActorInfo와 MoveSpeed→MaxWalkSpeed 기본 파생은 베이스(AFTCharacterBase)가 Super에서 처리한다.
		// 플레이어는 추가 속도 속성(스프린트·앉기 배수)만 더 듣는다 — 변경 시 베이스의 OnSpeedAttributeChanged가 ApplyMovementSpeed(override)를 재호출.
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTPlayerAttributeSet::GetSprintSpeedMultiplierAttribute())
			.AddUObject(this, &AFTPlayerCharacter::OnSpeedAttributeChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTPlayerAttributeSet::GetCrouchSpeedMultiplierAttribute())
			.AddUObject(this, &AFTPlayerCharacter::OnSpeedAttributeChanged);

		// 스턴 시 이동 정지/복원은 베이스(AFTCharacterBase)가 처리한다.
	}

	// 사망 통지(OnOutOfHealth → HandleDeath)와 초기 ApplyMovementSpeed 호출은 베이스가 Super에서 처리한다(플레이어 override 실행).

	// 서 있을 때의 카메라 붐 상대 위치를 앉기 보간의 기준점으로 캐시한다.
	if (CameraBoom)
	{
		DefaultBoomRelativeLocation = CameraBoom->GetRelativeLocation();
	}

}

void AFTPlayerCharacter::ApplyInitialAbilitySystemAttributes()
{
	if (!AbilitySystemComponent || !AttributeSet || !PlayerAttributeSet)
	{
		return;
	}

	const float MaxHealth = FMath::Max(InitialAttributes.MaxHealth, 0.0f);
	const float MaxStamina = FMath::Max(InitialAttributes.MaxStamina, 0.0f);

	// 최대값을 먼저 적용해야 현재값의 클램프가 에디터 설정을 기준으로 동작한다.
	AbilitySystemComponent->SetNumericAttributeBase(UFTAttributeSet::GetMaxHealthAttribute(), MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(
		UFTAttributeSet::GetHealthAttribute(),
		FMath::Clamp(InitialAttributes.Health, 0.0f, MaxHealth));
	AbilitySystemComponent->SetNumericAttributeBase(
		UFTAttributeSet::GetMoveSpeedAttribute(),
		FMath::Max(InitialAttributes.MoveSpeed, 0.0f));

	AbilitySystemComponent->SetNumericAttributeBase(UFTPlayerAttributeSet::GetMaxStaminaAttribute(), MaxStamina);
	AbilitySystemComponent->SetNumericAttributeBase(
		UFTPlayerAttributeSet::GetStaminaAttribute(),
		FMath::Clamp(InitialAttributes.Stamina, 0.0f, MaxStamina));
	AbilitySystemComponent->SetNumericAttributeBase(
		UFTPlayerAttributeSet::GetDexterityAttribute(),
		FMath::Max(InitialAttributes.Dexterity, 0.0f));
	AbilitySystemComponent->SetNumericAttributeBase(
		UFTPlayerAttributeSet::GetSprintSpeedMultiplierAttribute(),
		FMath::Max(InitialAttributes.SprintSpeedMultiplier, 0.0f));
	AbilitySystemComponent->SetNumericAttributeBase(
		UFTPlayerAttributeSet::GetCrouchSpeedMultiplierAttribute(),
		FMath::Max(InitialAttributes.CrouchSpeedMultiplier, 0.0f));

	// 배수 변경 delegate는 아래 BeginPlay 배선보다 먼저 적용되므로, 모든 값을 쓴 뒤 한 번 명시적으로 동기화한다.
	ApplyMovementSpeed();
}

void AFTPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 손에 어태치된 아이템 액터는 캐릭터 파괴 시 자동으로 정리되지 않으므로 직접 제거한다.
	if (HeldItemActor)
	{
		HeldItemActor->Destroy();
		HeldItemActor = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AFTPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 앉기 카메라 보정량을 0으로 부드럽게 보간.
	if (!FMath::IsNearlyZero(CrouchCameraOffsetZ))
	{
		CrouchCameraOffsetZ = FMath::FInterpTo(CrouchCameraOffsetZ, 0.0f, DeltaSeconds, CrouchCameraInterpSpeed);
		UpdateCrouchCameraOffset();
	}

	// 스프린트(스태미나 소모) → 회복 순서로 처리(소모가 회복 지연 타이머를 리셋).
	UpdateSprintState(DeltaSeconds);
	UpdateStaminaRegen(DeltaSeconds);
}

void AFTPlayerCharacter::HandleMoveInput(const FVector2D& MoveValue)
{
	// '연타로 탈출 가능한' 상태에서는 이동 대신 좌우 연타를 발버둥 입력으로 흘려보낸다.
	// 캡처는 State.Captured, GE 기반 탈출형 디버프(버블/빙결 등)는 State.Debuff.Escapable로 분리해 판정한다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(TAG_FT_State_Captured)
			|| ASC->HasMatchingGameplayTag(TAG_FT_State_Debuff_Escapable))
		{
			SendStruggleOnFlip(MoveValue.X);
			return;
		}
	}

	if (!Controller)
	{
		return;
	}

	// 이동하면 "시전형(.Channeled)" 아이템 동작만 취소한다(채널링 중단). 조준형 투척(.Aimed)은 달리며도 유지된다.
	if (!MoveValue.IsNearlyZero())
	{
		CancelItemUseAbilities(TAG_FT_Ability_ItemUse_Channeled);

		// 채널형 상호작용(진열대 털기 등)도 같은 규칙으로 이동에 끊긴다.
		// 키를 떼도 유지되는 토글 방식이므로, 그 자리를 떠나는 순간 작업이 풀리는 게 유일하게 자연스러운 해제다.
		if (InteractionComponent)
		{
			InteractionComponent->StopInteract();
		}
	}

	// UE 표준 컨벤션: MoveValue.Y = 전방, MoveValue.X = 우측. 축 구성은 IMC에서 맞춘다.
	const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix YawMatrix(YawRotation);

	if (!FMath::IsNearlyZero(MoveValue.Y))
	{
		AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), MoveValue.Y);
	}

	if (!FMath::IsNearlyZero(MoveValue.X))
	{
		AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), MoveValue.X);
	}
}

void AFTPlayerCharacter::HandleLookInput(const FVector2D& LookValue)
{
	// 붙잡힘/채널링 중에도 시점은 자유롭게 돌릴 수 있다(DBD식). 몸(캡슐)은 그동안 bUseControllerRotationYaw를
	// 꺼둬 제자리에 고정되므로, 시점만 스프링암(bUsePawnControlRotation)으로 컨트롤 회전을 따라 돈다.
	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);
}

void AFTPlayerCharacter::HandleJumpPressed()
{
	if (IsCaptured() || IsChannelingInteraction())
	{
		return;
	}

	if (bTryTraversalBeforeJump && TryStartTraversal())
	{
		return;
	}

	Jump();
}

void AFTPlayerCharacter::HandleJumpReleased()
{
	StopJumping();
}

void AFTPlayerCharacter::HandleSprintPressed()
{
	if (IsCaptured())
	{
		return;
	}

	// 키 상태만 기록. 조건 확인과 속도 적용은 Tick의 UpdateSprintState가 처리.
	bSprintHeld = true;
}

void AFTPlayerCharacter::HandleSprintReleased()
{
	bSprintHeld = false;
}

void AFTPlayerCharacter::HandleCrouchPressed()
{
	if (IsCaptured())
	{
		return;
	}

	Crouch();
}

void AFTPlayerCharacter::HandleCrouchReleased()
{
	UnCrouch();
}

void AFTPlayerCharacter::HandleInteractPressed()
{
	if (IsCaptured())
	{
		return;
	}

	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AFTPlayerCharacter::HandleInteractReleased()
{
	// 채널형 상호작용은 토글이라 키를 떼는 것으로는 끊기지 않는다(꾹 누르기 불필요).
	// 중단은 이동(HandleMoveInput) · 재입력(TryInteract) · 범위 이탈(UFTInteractionComponent::TickComponent)이 담당한다.
}

void AFTPlayerCharacter::HandleSkillCheckPressed()
{
	if (IsCaptured())
	{
		return;
	}

	if (InteractionComponent)
	{
		InteractionComponent->NotifySkillCheckInput();
	}
}

void AFTPlayerCharacter::HandleUseItemPressed()
{
	if (IsCaptured())
	{
		return;
	}

	UseInventoryItem(CurrentHeldInventoryItem);
}

void AFTPlayerCharacter::HandleUseItemReleased()
{
	if (IsCaptured())
	{
		return;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	// 손을 뗀 순간을 제네릭 이벤트로 알린다. 충전형(투척) 어빌리티가 활성 중이면 이걸 받아 실제 발동한다.
	// 즉시형 아이템은 이미 종료돼 있어 무해한 no-op이다(이 태그는 트리거 태그가 아니라 어떤 어빌리티도 새로 발동시키지 않는다).
	FGameplayEventData Payload;
	Payload.EventTag = TAG_FT_Event_UseReleased;
	Payload.Instigator = this;
	Payload.Target = this;
	AbilitySystemComponent->HandleGameplayEvent(TAG_FT_Event_UseReleased, &Payload);
}

void AFTPlayerCharacter::CancelItemUseAbilities(FGameplayTag MatchTag)
{
	// State.UsingItem(ActivationOwnedTags)으로 "아이템 동작이 진행 중인가"를 값싸게 가드한 뒤,
	// 실제 취소는 MatchTag(AssetTag) 매칭으로 한다(CancelAbilities는 AssetTags를 본다).
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(TAG_FT_State_UsingItem))
	{
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(MatchTag);
		AbilitySystemComponent->CancelAbilities(&CancelTags);
	}
}

void AFTPlayerCharacter::HandleSelectQuickSlot(int32 SlotIndex)
{
	if (IsCaptured())
	{
		return;
	}

	// 퀵슬롯 입력이 오면 진행 중인 아이템 동작을 종류 불문 취소한다(부모 Ability.ItemUse = .Channeled/.Aimed 모두 매칭).
	// 슬롯을 바꾸든 같은 슬롯을 다시 눌러 집어넣든, 조준 중이던 투척은 던지지 않고 취소된다.
	CancelItemUseAbilities(TAG_FT_Ability_ItemUse);

	UFTInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	// 인벤토리 열림 = 퀵슬롯 편집 모드: 누른 번호(SlotIndex)는 "대상 퀵슬롯 번호"일 뿐이고,
	// 실제로 등록되는 아이템은 인벤토리 N번째가 아니라 현재 UI에서 선택된 아이템(ViewModel->SelectedItem)이다.
	if (IsInventoryOpen())
	{
		const UFTUIManagerSubsystem* UIManager = GetUIManager();
		if (UFTInventoryViewModel* ViewModel = UIManager ? UIManager->InventoryViewModel : nullptr)
		{
			const bool bRegistered = ViewModel->RegisterSelectedToQuickSlot(SlotIndex);
			UE_LOG(LogFTPlayer, Verbose, TEXT("QuickSlot %d register selected -> %s."),
				SlotIndex, bRegistered ? TEXT("OK") : TEXT("no selection/rejected"));
		}
		return;
	}

	FFTInventoryItem QuickSlotItem;
	if (Inventory->GetQuickSlotItem(SlotIndex, QuickSlotItem) && QuickSlotItem.Quantity > 0 && QuickSlotItem.ItemDataAsset)
	{
		// 소모성 아이템(회복약 등)인 경우 장착하지 않고 즉시 사용
		UFTItemDataAsset* Item = QuickSlotItem.ItemDataAsset.Get();
		if (Item && Item->ItemData.CategoryType == EFTItemCategoryType::Healing)
		{
			UseInventoryItem(QuickSlotItem);
			return;
		}

		// 현재 선택 중인 퀵슬롯을 다시 입력하면 선택 해제(아이템 집어넣기)
		if (CurrentHeldInventoryItem.ItemId == QuickSlotItem.ItemId)
		{
			SetCurrentHeldInventoryItem(FFTInventoryItem());
			UE_LOG(LogFTPlayer, Verbose, TEXT("QuickSlot %d unequipped '%s' on '%s'."),
				SlotIndex, *QuickSlotItem.ItemId.ToString(), *GetName());
			return;
		}

		SetCurrentHeldInventoryItem(QuickSlotItem);
		EnsureUseAbilityGranted(CurrentHeldInventoryItem.ItemDataAsset->ItemData.UseData.UseAbility);
		UE_LOG(LogFTPlayer, Verbose, TEXT("QuickSlot %d equipped '%s' on '%s'."),
			SlotIndex, *CurrentHeldInventoryItem.ItemId.ToString(), *GetName());
	}
	else
	{
		SetCurrentHeldInventoryItem(FFTInventoryItem());
	}
}

void AFTPlayerCharacter::HandleToggleInventoryPressed()
{
	if (IsCaptured())
	{
		return;
	}

	// 열림 상태의 단일 소스는 UI 서브시스템이다. 캐릭터는 토글만 위임하고 상태는 보유하지 않는다.
	if (UFTUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->ToggleInventory();
	}
	UE_LOG(LogFTPlayer, Verbose, TEXT("Inventory toggled -> %d"), IsInventoryOpen());
}

bool AFTPlayerCharacter::IsCaptured() const
{
	return CaptureEscapeComponent && CaptureEscapeComponent->IsCaptured();
}

void AFTPlayerCharacter::SendStruggleOnFlip(float MoveAxisX)
{
	// 데드존 밖일 때만 방향으로 인정. 직전 인정 방향과 반대가 되면 "좌우 전환(flip)" 1회로 Event.Struggle 발행.
	float Sign = 0.0f;
	if (MoveAxisX > StruggleInputDeadzone)
	{
		Sign = 1.0f;
	}
	else if (MoveAxisX < -StruggleInputDeadzone)
	{
		Sign = -1.0f;
	}

	if (Sign == 0.0f)
	{
		return;
	}

	if (StruggleLastSign != 0.0f && Sign != StruggleLastSign)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			// 자기 ASC로 발행 → 현재 활성인 탈출 시스템(캡처 컴포넌트 또는 GE 기반 탈출 GA)이 게이지를 올린다.
			FGameplayEventData Payload;
			Payload.EventTag = TAG_FT_Event_Struggle;
			Payload.Instigator = this;
			ASC->HandleGameplayEvent(TAG_FT_Event_Struggle, &Payload);
		}
	}
	StruggleLastSign = Sign;
}

bool AFTPlayerCharacter::IsInventoryOpen() const
{
	const UFTUIManagerSubsystem* UIManager = GetUIManager();
	return UIManager && UIManager->IsInventoryOpen();
}

UFTUIManagerSubsystem* AFTPlayerCharacter::GetUIManager() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UFTUIManagerSubsystem>() : nullptr;
}

bool AFTPlayerCharacter::IsChannelingInteraction() const
{
	// 채널 상태의 단일 출처는 InteractionComponent다(여기선 AnimBP가 폰에서 바로 읽도록 중계만 한다).
	return InteractionComponent && InteractionComponent->IsChanneling();
}

EFTWeaponStanceType AFTPlayerCharacter::GetHeldWeaponStance() const
{
	// 스탠스의 단일 출처는 손에 든 아이템 데이터다. 든 게 없으면 맨손.
	const UFTItemDataAsset* Item = CurrentHeldInventoryItem.ItemDataAsset.Get();
	return Item ? Item->ItemData.WeaponStance : EFTWeaponStanceType::Unarmed;
}

void AFTPlayerCharacter::SetCurrentHeldInventoryItem(const FFTInventoryItem& NewHeldItem, bool bPlaySound)
{
	// 같은 아이템(ItemId 동일)이면 비주얼 액터를 다시 스폰하지 않는다.
	// 같은 퀵슬롯을 반복해서 누를 때 Destroy→Respawn으로 깜빡이거나 진행 중인 연출이 끊기는 것을 막는다.
	// 수량/데이터 포인터는 갱신될 수 있으므로 값 자체는 덮어쓴다.
	const bool bHeldItemChanged = (CurrentHeldInventoryItem.ItemId != NewHeldItem.ItemId);

	// 집어넣는 소리는 '나가는' 아이템의 것이다 — 아래에서 값이 덮여 쓰이기 전에 미리 읽어둔다.
	const UFTItemDataAsset* OutgoingItemData = bHeldItemChanged ? CurrentHeldInventoryItem.ItemDataAsset.Get() : nullptr;

	CurrentHeldInventoryItem = NewHeldItem;

	if (CrosshairComponent)
	{
		CrosshairComponent->SetActiveItemData(CurrentHeldInventoryItem.ItemDataAsset.Get());
	}

	if (bHeldItemChanged)
	{
		RefreshHeldItemActor();

		// 손에 든 것이 실제로 바뀐 순간에만 낸다. 같은 슬롯을 다시 눌러 값만 갱신되는 경우엔 위 가드에 걸려 조용하다.
		// 교체(A→B)면 집어넣는 소리와 꺼내는 소리가 함께 나 자연스러운 홀스터→드로우가 된다.
		if (bPlaySound)
		{
			PlayHeldItemSound(OutgoingItemData, /*bEquipped=*/false);
			PlayHeldItemSound(CurrentHeldInventoryItem.ItemDataAsset.Get(), /*bEquipped=*/true);
		}
	}
}

void AFTPlayerCharacter::PlayHeldItemSound(const UFTItemDataAsset* ItemData, bool bEquipped) const
{
	if (!ItemData)
	{
		return;
	}

	USoundBase* SoundToPlay = bEquipped ? ItemData->ItemData.EquipSound : ItemData->ItemData.UnequipSound;
	if (!SoundToPlay)
	{
		return;
	}

	// 사용자에 붙여 재생 — 걸어가며 무기를 바꿔도 소리가 몸을 따라간다(피격음/발소리/아이템 사용음과 같은 방식).
	UGameplayStatics::SpawnSoundAttached(SoundToPlay, GetRootComponent());
}

void AFTPlayerCharacter::RefreshHeldItemActor()
{
	// 교체/해제 공통: 이전에 들고 있던 액터는 항상 먼저 제거한다.
	if (HeldItemActor)
	{
		HeldItemActor->Destroy();
		HeldItemActor = nullptr;
	}

	UFTItemDataAsset* ItemData = CurrentHeldInventoryItem.ItemDataAsset.Get();
	UWorld* World = GetWorld();
	USkeletalMeshComponent* OwnerMesh = GetMesh();

	// 들 아이템이 없거나(빈 슬롯/해제) 스폰 조건이 안 되면 손을 비운 상태로 둔다.
	if (CurrentHeldInventoryItem.ItemId.IsNone() || !ItemData || !World || !OwnerMesh || !HeldItemActorClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	// 손에 붙일 비주얼이라 충돌로 스폰이 실패하면 안 된다.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	HeldItemActor = World->SpawnActor<AFTItemActor>(HeldItemActorClass, GetActorTransform(), SpawnParams);
	if (!HeldItemActor)
	{
		return;
	}

	// 데이터 주입 후 외형 갱신.
	HeldItemActor->ItemData = ItemData;
	HeldItemActor->UpdateAppearance();

	// 손에 든 인스턴스는 월드 픽업이 아니므로 물리/충돌/오버랩을 끈다.
	// (AFTItemActor는 기본적으로 물리 시뮬레이션 + 줍기 가능 상태로 생성되므로, 그대로 두면 흔들리거나 자기 손에서 다시 줍힌다.)
	HeldItemActor->SetActorEnableCollision(false);

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	HeldItemActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetSimulatePhysics(false);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PrimitiveComponent->SetGenerateOverlapEvents(false);
	}

	// 아이템 타입별 데이터에서 어태치 소켓을 결정한다(없으면 폴백).
	const FName AttachSocketName = ResolveHeldItemAttachSocket(ItemData);
	if (!AttachSocketName.IsNone() && !OwnerMesh->DoesSocketExist(AttachSocketName))
	{
		UE_LOG(LogFTItem, Warning, TEXT("Held item socket '%s' does not exist on mesh '%s'. Attaching to mesh root."),
			*AttachSocketName.ToString(), *GetNameSafe(OwnerMesh));
	}

	HeldItemActor->AttachToComponent(
		OwnerMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		OwnerMesh->DoesSocketExist(AttachSocketName) ? AttachSocketName : NAME_None);

	UE_LOG(LogFTItem, Verbose, TEXT("Held item actor spawned. Item=%s Socket=%s Actor=%s"),
		*GetNameSafe(ItemData), *AttachSocketName.ToString(), *GetNameSafe(HeldItemActor));
}

FName AFTPlayerCharacter::ResolveHeldItemAttachSocket(const UFTItemDataAsset* ItemData) const
{
	// 무기 타입별 데이터 에셋이 각자 어태치 소켓을 들고 있으므로 그 값을 우선 사용한다.
	if (const UFTMeleeDataAsset* MeleeData = Cast<UFTMeleeDataAsset>(ItemData))
	{
		if (!MeleeData->MeleeActionData.AttachSocketName.IsNone())
		{
			return MeleeData->MeleeActionData.AttachSocketName;
		}
	}
	else if (const UFTHitScanDataAsset* HitScanData = Cast<UFTHitScanDataAsset>(ItemData))
	{
		if (!HitScanData->HitScanActionData.AttachSocketName.IsNone())
		{
			return HitScanData->HitScanActionData.AttachSocketName;
		}
	}
	
	// else if (const UFTProjectileDataAsset* ProjectileData = Cast<UFTProjectileDataAsset>(ItemData))
	// {
	// 	if (!ProjectileData->ProjectileAttackData.AttachSocketName.IsNone())
	// 	{
	// 		return ProjectileData->ProjectileAttackData.AttachSocketName;
	// 	}
	// }
	
	else if (const UFTLauncherDataAsset* LauncherData = Cast<UFTLauncherDataAsset>(ItemData))
	{
		if (!LauncherData->LauncherActionData.AttachSocketName.IsNone())
		{
			return LauncherData->LauncherActionData.AttachSocketName;
		}
	}
	else if (const UFTThrowDataAsset* ThrowData = Cast<UFTThrowDataAsset>(ItemData))
	{
		if (!ThrowData->ThrowActorData.AttachSocketName.IsNone())
		{
			return ThrowData->ThrowActorData.AttachSocketName;
		}
	}

	// 타입 미지정이거나 소켓이 비어 있으면 폴백 소켓을 쓴다.
	return HeldItemFallbackSocketName;
}

void AFTPlayerCharacter::OnInventoryChangedCallback()
{
	UFTInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory || CurrentHeldInventoryItem.ItemId.IsNone())
	{
		return;
	}

	if (Inventory->GetItemQuantity(CurrentHeldInventoryItem.ItemId) <= 0)
	{
		// 다 써서 손에서 없어진 것이지 플레이어가 집어넣은 게 아니므로 집어넣는 소리는 내지 않는다.
		SetCurrentHeldInventoryItem(FFTInventoryItem(), /*bPlaySound=*/false);
	}
}

void AFTPlayerCharacter::HandleActiveChannelChanged(AActor* ChannelTarget)
{
	// 채널 중엔 몸(캡슐 yaw)이 컨트롤 회전을 따라 돌지 않게 끈다. 카메라 붐은 bUsePawnControlRotation으로
	// 계속 컨트롤 회전을 따르므로, 작업하던 방향에 몸을 둔 채 시점만 주위를 둘러보는 그림이 된다.
	// 채널이 끝나면 즉시 다시 따라 돌게 해 그 프레임부터 조준형 조작이 원래대로 복구된다.
	// 저장/복원 대신 상태에서 다시 계산하는 이유: 캡처(UFTCaptureEscapeComponent)도 같은 플래그를 저장/복원하는데,
	// 그쪽 스냅샷 시점에 이 값이 false로 눌려 있으면 캡처가 끝나며 false가 영구히 굳는다.
	// 그래서 캡처 시작이 진행 중인 채널을 먼저 끊어 두 소유자가 겹치지 않게 한다(TryBeginCapture 참조).
	bUseControllerRotationYaw = (ChannelTarget == nullptr);
}

void AFTPlayerCharacter::ApplyMovementSpeed()
{
	if (!AttributeSet || !PlayerAttributeSet)
	{
		return;
	}

	// 기본속도는 공용 MoveSpeed(버프 포함 최종값), 스프린트·앉기 배수는 플레이어 전용 속성에서 파생한다.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		const float BaseSpeed = AttributeSet->GetMoveSpeed();
		Movement->MaxWalkSpeed = bIsSprinting ? BaseSpeed * PlayerAttributeSet->GetSprintSpeedMultiplier() : BaseSpeed;
		Movement->MaxWalkSpeedCrouched = BaseSpeed * PlayerAttributeSet->GetCrouchSpeedMultiplier();
	}
}

float AFTPlayerCharacter::GetSprintMovementSpeed() const
{
	const float BaseSpeed = AttributeSet ? AttributeSet->GetMoveSpeed() : InitialAttributes.MoveSpeed;
	const float Multiplier = PlayerAttributeSet
		? PlayerAttributeSet->GetSprintSpeedMultiplier()
		: InitialAttributes.SprintSpeedMultiplier;
	return FMath::Max(BaseSpeed * Multiplier, 0.0f);
}

float AFTPlayerCharacter::GetCrouchMovementSpeed() const
{
	const float BaseSpeed = AttributeSet ? AttributeSet->GetMoveSpeed() : InitialAttributes.MoveSpeed;
	const float Multiplier = PlayerAttributeSet
		? PlayerAttributeSet->GetCrouchSpeedMultiplier()
		: InitialAttributes.CrouchSpeedMultiplier;
	return FMath::Max(BaseSpeed * Multiplier, 0.0f);
}

void AFTPlayerCharacter::UpdateSprintState(float DeltaSeconds)
{
	if (!AbilitySystemComponent || !PlayerAttributeSet)
	{
		return;
	}

	const float Stamina = PlayerAttributeSet->GetStamina();
	const float MaxStamina = PlayerAttributeSet->GetMaxStamina();

	// 탈진 해제: 스태미나가 최대치의 SprintResumeStaminaFraction 이상으로 회복되면 다시 스프린트 가능.
	if (bSprintExhausted && Stamina >= MaxStamina * SprintResumeStaminaFraction)
	{
		bSprintExhausted = false;
	}
	
	bool bSprinting = bSprintHeld && !bIsCrouched && !bSprintExhausted && Stamina > 0.0f;

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const bool bMovingOnGround = Movement && Movement->IsMovingOnGround() && GetVelocity().SizeSquared() > FMath::Square(10.0f);
	if (bSprinting && bMovingOnGround)
	{
		// 스태미나 속성을 직접 감소(클램프는 PreAttributeChange가 처리). 소모 시 회복 지연 타이머 리셋.
		AbilitySystemComponent->ApplyModToAttribute(UFTPlayerAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -SprintStaminaCostPerSecond * DeltaSeconds);
		TimeSinceStaminaUse = 0.0f;

		if (PlayerAttributeSet->GetStamina() <= 0.0f)
		{
			bSprintExhausted = true;
			bSprinting = false;
		}
	}

	if (bSprinting != bIsSprinting)
	{
		bIsSprinting = bSprinting;
		ApplyMovementSpeed();
	}
}

void AFTPlayerCharacter::UpdateStaminaRegen(float DeltaSeconds)
{
	if (!AbilitySystemComponent || !AttributeSet || !PlayerAttributeSet)
	{
		return;
	}

	TimeSinceStaminaUse += DeltaSeconds;

	// 스태미나 회복(마지막 사용 후 지연이 지난 다음부터).
	if (StaminaRegenPerSecond > 0.0f && TimeSinceStaminaUse >= StaminaRegenDelay
		&& PlayerAttributeSet->GetStamina() < PlayerAttributeSet->GetMaxStamina())
	{
		AbilitySystemComponent->ApplyModToAttribute(UFTPlayerAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, StaminaRegenPerSecond * DeltaSeconds);
	}

	// 체력 회복(기본 0이라 보통 비활성).
	if (HealthRegenPerSecond > 0.0f && AttributeSet->GetHealth() > 0.0f
		&& AttributeSet->GetHealth() < AttributeSet->GetMaxHealth())
	{
		AbilitySystemComponent->ApplyModToAttribute(UFTAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, HealthRegenPerSecond * DeltaSeconds);
	}
}

void AFTPlayerCharacter::OnDeath()
{
	UE_LOG(LogFTPlayer, Log, TEXT("'%s' died (health depleted)."), *GetNameSafe(this));

	// 입력 차단: 이동/시점/점프/아이템 등 모든 입력은 컨트롤러의 InputComponent에 바인딩돼 있으므로
	// 컨트롤러에 대해 DisableInput을 호출해야 한다(BuildInputStack이 컨트롤러의 InputEnabled로 게이트).
	// 폰에 대한 DisableInput은 이 입력들을 막지 못한다. 이동 정지/능력 취소는 베이스가 이미 처리했다.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->DisableInput(PC);
	}
	
	FFTMessagePayloadStruct Payload;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_FT_Event_PlayerDead, Payload);
	MessageSubsystem.BroadcastMessage(TAG_FT_Request_Flow_FailRaid, Payload);

	// 게임오버/리스폰/레벨 전환은 GameFlow 연동으로 — 이번 스코프 밖.
}

UFTInventoryComponent* AFTPlayerCharacter::GetInventoryComponent() const
{
	return FindComponentByClass<UFTInventoryComponent>();
}

bool AFTPlayerCharacter::EnsureUseAbilityGranted(TSubclassOf<UFTGameplayAbility> UseAbility)
{
	if (!AbilitySystemComponent || !UseAbility)
	{
		return false;
	}

	if (AbilitySystemComponent->FindAbilitySpecFromClass(UseAbility))
	{
		return true;
	}

	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UseAbility));
	return AbilitySystemComponent->FindAbilitySpecFromClass(UseAbility) != nullptr;
}

void AFTPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ApplyCrouchCameraCompensation(+HalfHeightAdjust);
}

void AFTPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ApplyCrouchCameraCompensation(-HalfHeightAdjust);
}

bool AFTPlayerCharacter::CanJumpInternal_Implementation() const
{
	if (!Super::CanJumpInternal_Implementation())
	{
		return false;
	}

	// 이미 점프 중(JumpMaxHoldTime 동안 가변 높이를 유지하는 구간)이면 비용은 이륙 때 이미 냈다.
	// 여기서 다시 검사하면 방금 깎인 스태미나 때문에 상승이 중간에 끊기므로 통과시킨다.
	if (bWasJumping)
	{
		return true;
	}

	return HasEnoughStaminaForJump();
}

void AFTPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// 점프가 성립한 뒤에만 여기 도달하므로(CanJump 통과 + DoJump 성공), 헛도는 입력엔 스태미나가 나가지 않는다.
	if (JumpStaminaCost > 0.0f && AbilitySystemComponent)
	{
		AbilitySystemComponent->ApplyModToAttribute(UFTPlayerAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, -JumpStaminaCost);
		TimeSinceStaminaUse = 0.0f;
	}
}

bool AFTPlayerCharacter::HasEnoughStaminaForJump() const
{
	if (JumpStaminaCost <= 0.0f)
	{
		return true;
	}

	if (!PlayerAttributeSet)
	{
		return true;
	}

	return PlayerAttributeSet->GetStamina() >= JumpStaminaCost;
}

void AFTPlayerCharacter::ApplyCrouchCameraCompensation(float CameraOffsetDeltaZ)
{
	// 엔진이 base 고정 시(주로 지상)에만 캡슐 중심을 옮기므로, 그 경우에만 반대로 보정한다.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || !Movement->bCrouchMaintainsBaseLocation)
	{
		return;
	}

	CrouchCameraOffsetZ += CameraOffsetDeltaZ;
	UpdateCrouchCameraOffset();
}

void AFTPlayerCharacter::UpdateCrouchCameraOffset()
{
	if (CameraBoom)
	{
		CameraBoom->SetRelativeLocation(DefaultBoomRelativeLocation + FVector(0.0f, 0.0f, CrouchCameraOffsetZ));
	}
}

void AFTPlayerCharacter::InitializeCrouchCapsuleSize()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Capsule || !Movement)
	{
		return;
	}

	InitialCapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
	InitialCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();

	// 캡슐 Half Height는 Radius보다 작을 수 없고, 크라우치가 서 있는 캡슐보다 커지지 않게 제한한다.
	const float RequestedHalfHeight = InitialCapsuleHalfHeight * FMath::Clamp(CrouchCapsuleHeightRatio, 0.0f, 1.0f);
	const float CrouchedHalfHeight = FMath::Clamp(RequestedHalfHeight, InitialCapsuleRadius, InitialCapsuleHalfHeight);
	Movement->SetCrouchedHalfHeight(CrouchedHalfHeight);
}

bool AFTPlayerCharacter::TryStartTraversal()
{
	// 지상에서만, 그리고 이미 트래버설 중이 아닐 때만 시도한다. 컴포넌트가 장애물을 못 찾으면 false → 일반 점프.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!TraversalComponent || TraversalComponent->IsTraversing() || !Movement || !Movement->IsMovingOnGround())
	{
		return false;
	}

	return TraversalComponent->TryTraversal();
}

void AFTPlayerCharacter::UseInventoryItem(const FFTInventoryItem& InventoryItem)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	UFTItemDataAsset* Item = InventoryItem.ItemDataAsset.Get();
	if (!Item || !Item->ItemData.UseData.UseAbility)
	{
		return;
	}

	UFTInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory || InventoryItem.ItemId.IsNone()
		|| Inventory->GetItemQuantity(InventoryItem.ItemId) <= 0)
	{
		// 사용하려던 아이템이 손에 쥐고 있던 템인데 다 소진되었다면 장착 해제
		// (소모로 사라진 것이라 집어넣는 소리는 내지 않는다 — 소모 연출은 사용음의 몫이다.)
		if (InventoryItem.ItemId == CurrentHeldInventoryItem.ItemId)
		{
			SetCurrentHeldInventoryItem(FFTInventoryItem(), /*bPlaySound=*/false);
		}
		return;
	}

	if (!EnsureUseAbilityGranted(Item->ItemData.UseData.UseAbility))
	{
		return;
	}

	// 발동할 어빌리티가 선언한 트리거 태그를 그 CDO에서 읽어, 그 태그로만 이벤트를 보낸다.
	const UFTGameplayAbility* AbilityCDO = Item->ItemData.UseData.UseAbility.GetDefaultObject();
	const FGameplayTag EventTag = AbilityCDO ? AbilityCDO->GetTriggerEventTag() : FGameplayTag();
	if (!EventTag.IsValid())
	{
		return;
	}
	
	// 아이템별 쿨다운 차단
	const FTItemUseStruct& UseData = Item->ItemData.UseData;
	if (UseData.CooldownSeconds > 0.0f
		&& AbilitySystemComponent->HasMatchingGameplayTag(UFTGA_ItemAbility::ResolveCooldownTag(UseData)))
	{
		return;
	}

	// 효과/시전/쿨다운/수치는 아이템 데이터(UseData)에 있고, 어빌리티가 페이로드에서 읽어 처리한다.
	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = this;
	Payload.Target = this;
	Payload.OptionalObject = Item;
	AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);
}
