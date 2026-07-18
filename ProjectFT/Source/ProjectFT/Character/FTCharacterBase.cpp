// Fill out your copyright notice in the Description page of Project Settings.

#include "FTCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_BubbleStackTrap.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_EscapableDebuff.h"
#include "ProjectFT/Struct/FTCharacterAttackedPayloadStruct.h"

AFTCharacterBase::AFTCharacterBase()
{
	// GAS: 능력시스템 컴포넌트 + 공용 속성셋. 속성셋은 캐릭터 서브오브젝트라 ASC가 자동 등록한다.
	// (서브클래스가 추가 속성셋을 더 만들면 그 세트도 같은 ASC에 자동 등록된다.)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UFTAttributeSet>(TEXT("AttributeSet"));

	// CMC의 초기 MaxWalkSpeed(CDO/프리뷰)를 MoveSpeed 속성 기본값에서 가져온다 — 자식이 ctor에서 따로 셋하지 않게 단일화.
	// (런타임엔 BeginPlay의 ApplyMovementSpeed가 최종값으로 다시 덮어쓴다.)
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
	}

	// 무기 트레이스(Weapon = ECC_GameTraceChannel1)가 캡슐을 '통과'해 메시에 정밀히 맞도록 캡슐만 Ignore로 둔다.
	// 캡슐 기본 Weapon 응답은 Block이라 명시적으로 덮어씀 — 메시는 CharacterMesh 프로파일(QueryOnly + Weapon 기본 Block)이라 그대로 맞는다.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	}

}

UAbilitySystemComponent* AFTCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AFTCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// 싱글: 소유자=아바타=this. (InitializeComponent가 한 번 호출하지만 명시적으로 한 번 더 — 안전.)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// MoveSpeed 속성 → MaxWalkSpeed 반영(Slow/Haste가 이동에 보이도록). 모든 캐릭터 공통 기본 파생을 베이스가 제공한다.
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UFTAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &AFTCharacterBase::OnSpeedAttributeChanged);

		// 행동불능 우산 태그 부착/해제 → 공통 이동 정지/복원. 개별 효과(스턴/마비/비눗방울 등)가 아니라
		// 우산 태그 하나만 감시하므로, 새 행동불능 효과가 추가돼도 이 코드는 바뀌지 않는다. 추가 반응은 OnImmobilizedStateChanged override.
		AbilitySystemComponent->RegisterGameplayTagEvent(TAG_FT_State_Debuff_Immobilized, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &AFTCharacterBase::OnImmobilizeTagChanged);

		// GE가 자신에게 적용될 때마다 통지받는다(instant/duration 모두). 적대적 GE(Effect.Hostile)면 데미지든 상태이상이든
		// 하나의 "공격당함" 신호로 수렴시킨다 — 모디파이어 없는 스턴 등도 여기서 잡힌다(PostGameplayEffectExecute 미호출).
		AbilitySystemComponent->OnGameplayEffectAppliedDelegateToSelf
			.AddUObject(this, &AFTCharacterBase::OnHostileEffectApplied);

		// 공통 어빌리티 부여. 트리거형이라 부여만으로 충분(상황에 맞게 자동 발동). 싱글이라 권한 검사 생략.
		// 비눗방울 갇힘/탈출 어빌리티는 어떤 캐릭터든 대상이 될 수 있으므로, BP의 CommonAbilities 설정과 무관하게 여기서 '항상' 보장한다.
		// (C++ 생성자 배열 기본값은 기존 BP에 전파가 불안정해서, 클래스 지정으로 직접 부여한다.)
		auto GrantAbilityOnce = [this](TSubclassOf<UGameplayAbility> AbilityClass)
		{
			if (AbilityClass && !AbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass))
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass));
			}
		};

		for (const TSubclassOf<UGameplayAbility>& AbilityClass : CommonAbilities)
		{
			GrantAbilityOnce(AbilityClass);
		}
		GrantAbilityOnce(UFTGA_BubbleStackTrap::StaticClass());
		GrantAbilityOnce(UFTGA_EscapableDebuff::StaticClass());
	}

	if (AttributeSet)
	{
		// 체력 0 도달 시 HandleDeath()로 통지 — 서브클래스가 사망 처리.
		AttributeSet->OnOutOfHealth.AddUObject(this, &AFTCharacterBase::HandleDeath);
	}

	// 초기 MoveSpeed를 MaxWalkSpeed에 반영(자식이 override 했으면 그 구현으로).
	ApplyMovementSpeed();
}

void AFTCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopStruggleJitter();
	Super::EndPlay(EndPlayReason);
}

void AFTCharacterBase::HandleDeath()
{
	// 재진입 가드: 0 HP 상태에서 체력 변경 GE(독 DoT 등)가 다시 실행돼 OnOutOfHealth가 재통지돼도 사망 처리는 1회만.
	if (bDead)
	{
		return;
	}
	bDead = true;

	if (AbilitySystemComponent)
	{
		// 사망 상태의 단일 소스. GE 수명이 아니라 캐릭터 상태이므로 Loose 태그로 직접 부여한다.
		AbilitySystemComponent->AddLooseGameplayTag(TAG_FT_State_Dead);

		// 진행 중이던 능력(아이템 사용/투척 등)을 즉시 취소한다.
		AbilitySystemComponent->CancelAllAbilities();
	}

	// 공통 이동 정지 — 모든 캐릭터는 죽으면 멈춘다(기존에 AI가 개별로 하던 것을 베이스로 통합).
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	// 자식별 사망 후처리(플레이어 입력 차단/게임오버, AI 래그돌/드롭/디스폰 등).
	OnDeath();
}

void AFTCharacterBase::OnDeath()
{
	// 기본 구현 없음. 자식이 사망 연출/후처리를 확장한다(공통 처리는 HandleDeath가 이미 수행).
}

void AFTCharacterBase::ApplyMovementSpeed()
{
	// 기본 파생: MaxWalkSpeed = MoveSpeed(버프 포함 최종값). 스프린트/앉기 등은 자식이 override.
	if (!AttributeSet)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = AttributeSet->GetMoveSpeed();
	}
}

void AFTCharacterBase::OnSpeedAttributeChanged(const FOnAttributeChangeData& Data)
{
	ApplyMovementSpeed();
}

void AFTCharacterBase::OnImmobilizeTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	// 우산 태그 카운트가 곧 '동시에 활성인 행동불능 수'다. 여러 효과가 겹쳐도 카운트로 합성되므로,
	// 콜백의 NewCount(우산 태그 카운트)가 0보다 크면 여전히 봉쇄 — 전부 사라져야(0) 복원된다.
	const bool bImmobilized = NewCount > 0;

	// 공통 반응: 행동불능이 되면 '진행 중이던' 아이템 동작도 끊는다. UFTGameplayAbility의 ActivationBlockedTags는
	// 새 발동만 막을 뿐 이미 도는 어빌리티엔 닿지 않아서, 행동불능 직전에 시작한 공격 몽타주가 계속 돌며 적중했다
	// (예: 잡히기 직전 휘두른 무기가 잡은 경비를 때려 그 자리에서 풀려나는 문제).
	// 취소 기준은 Ability.ItemUse 에셋 태그 — 행동불능 '중에' 돌아야 하는 탈출/트랩 어빌리티(UFTGA_EscapableDebuff,
	// UFTGA_BubbleStackTrap)는 아이템 동작이 아니라 여기 걸리지 않는다. 이동 정지보다 먼저 취소해야 어빌리티 종료가
	// 이동 모드를 되돌려놓아도 아래 봉쇄가 마지막 말이 된다.
	if (bImmobilized && AbilitySystemComponent)
	{
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(TAG_FT_Ability_ItemUse);
		AbilitySystemComponent->CancelAbilities(&CancelTags);
	}

	// 공통 반응: 행동불능 시작 시 현재 이동 모드를 저장하고 즉시 정지+이동 비활성, 해제 시 저장한 이동 모드로 복원.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (bImmobilized)
		{
			if (!bHasPreImmobilizedMovementMode)
			{
				PreImmobilizedMovementMode = Movement->MovementMode;
				PreImmobilizedCustomMovementMode = Movement->CustomMovementMode;
				bHasPreImmobilizedMovementMode = true;
			}
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		else
		{
			if (!bDead)
			{
				const EMovementMode MovementModeToRestore = bHasPreImmobilizedMovementMode
					? PreImmobilizedMovementMode.GetValue()
					: MOVE_Walking;
				const uint8 CustomMovementModeToRestore = bHasPreImmobilizedMovementMode
					? PreImmobilizedCustomMovementMode
					: 0;
				Movement->SetMovementMode(MovementModeToRestore, CustomMovementModeToRestore);
			}
			bHasPreImmobilizedMovementMode = false;
			PreImmobilizedMovementMode = MOVE_None;
			PreImmobilizedCustomMovementMode = 0;
		}
	}

	// 행동불능 '지속' 연출(GameplayCue)은 각 GE(GE_Stun 등)의 GameplayCues에 달려
	// GE 수명과 함께 자동 발동/제거된다(여기서 직접 Add/Remove하지 않는다 — 중복 발동 방지).
	OnImmobilizedStateChanged(bImmobilized);
}

void AFTCharacterBase::OnImmobilizedStateChanged(bool bImmobilized)
{
	// 기본 구현 없음. 자식이 AI 로직 정지/애니 등 추가 반응을 처리한다(이동 정지/복원은 베이스가 이미 처리).
}

void AFTCharacterBase::OnHostileEffectApplied(UAbilitySystemComponent* Source, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
	// 적대적 행동인지는 GE의 '에셋 태그'(Effect.Hostile)로 판정한다 — 데미지/스턴/슬로우/독/비눗방울 등 종류 불문 단일 기준.
	// (부여 태그 State.Debuff.*가 아니라 에셋 태그를 보는 이유: 순수 데미지 GE는 상태 태그를 부여하지 않아도 적대적이기 때문.)
	FGameplayTagContainer AssetTags;
	Spec.GetAllAssetTags(AssetTags);
	if (!AssetTags.HasTag(TAG_FT_Effect_Hostile))
	{
		return;
	}

	// 공격자는 이미 Spec의 EffectContext에 실려있다(더미 속성 불필요). 데미지 경로(PostGameplayEffectExecute)와 동일한 폴백 순서.
	const FGameplayEffectContextHandle& Context = Spec.GetContext();
	AActor* InstigatorActor = Context.GetOriginalInstigator();
	if (!InstigatorActor)
	{
		InstigatorActor = Context.GetEffectCauser();
	}
	if (!InstigatorActor)
	{
		InstigatorActor = Context.GetInstigator();
	}
	if (!InstigatorActor && Context.GetInstigatorAbilitySystemComponent())
	{
		InstigatorActor = Context.GetInstigatorAbilitySystemComponent()->GetAvatarActor();
	}

	FFTCharacterAttackedPayloadStruct Payload;
	Payload.InstigatorActor = InstigatorActor;
	Payload.TargetActor = this;
	// 리스너가 공격 종류를 분기할 수 있도록 에셋 태그 + 부여 태그를 병합해 담는다(예: State.Debuff.Stun 유무로 스턴 공격 판별).
	Payload.EffectTags = AssetTags;
	FGameplayTagContainer GrantedTags;
	Spec.GetAllGrantedTags(GrantedTags);
	Payload.EffectTags.AppendTags(GrantedTags);

	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(TAG_FT_Event_CharacterAttacked, Payload);
	}
}

void AFTCharacterBase::PlayStruggleJitter()
{
	if (bDead || StruggleJitterAmplitude <= 0.0f || StruggleJitterDuration <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!World || !CharacterMesh)
	{
		return;
	}

	if (StruggleJitterTimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(StruggleJitterTimerHandle);
	}
	ApplyStruggleJitterOffset(FVector::ZeroVector);

	StruggleJitterDirection *= -1;
	StruggleJitterElapsed = 0.0f;
	StruggleJitterLastUpdateTime = World->GetTimeSeconds();

	ApplyStruggleJitterOffset(FVector(0.0f, StruggleJitterAmplitude * static_cast<float>(StruggleJitterDirection), 0.0f));
	World->GetTimerManager().SetTimer(
		StruggleJitterTimerHandle,
		this,
		&AFTCharacterBase::UpdateStruggleJitter,
		StruggleJitterTickInterval,
		true);
}

void AFTCharacterBase::UpdateStruggleJitter()
{
	UWorld* World = GetWorld();
	if (!World || !GetMesh())
	{
		StopStruggleJitter();
		return;
	}

	const float Now = World->GetTimeSeconds();
	const float DeltaSeconds = StruggleJitterLastUpdateTime > 0.0f
		? FMath::Max(0.0f, Now - StruggleJitterLastUpdateTime)
		: StruggleJitterTickInterval;
	StruggleJitterLastUpdateTime = Now;
	StruggleJitterElapsed += DeltaSeconds;

	if (StruggleJitterElapsed >= StruggleJitterDuration)
	{
		StopStruggleJitter();
		return;
	}

	const float Alpha = FMath::Clamp(StruggleJitterElapsed / StruggleJitterDuration, 0.0f, 1.0f);
	const float Decay = 1.0f - Alpha;
	const float Oscillation = FMath::Cos(StruggleJitterElapsed * StruggleJitterFrequency * 2.0f * PI);
	const float OffsetY = StruggleJitterAmplitude * Decay * Oscillation * static_cast<float>(StruggleJitterDirection);
	ApplyStruggleJitterOffset(FVector(0.0f, OffsetY, 0.0f));
}

void AFTCharacterBase::StopStruggleJitter()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StruggleJitterTimerHandle);
	}

	ApplyStruggleJitterOffset(FVector::ZeroVector);
	StruggleJitterTimerHandle.Invalidate();
	StruggleJitterElapsed = 0.0f;
	StruggleJitterLastUpdateTime = 0.0f;
}

void AFTCharacterBase::ApplyStruggleJitterOffset(const FVector& NewOffset)
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		StruggleJitterAppliedOffset = FVector::ZeroVector;
		return;
	}

	const FVector BaseRelativeLocation = CharacterMesh->GetRelativeLocation() - StruggleJitterAppliedOffset;
	CharacterMesh->SetRelativeLocation(BaseRelativeLocation + NewOffset);
	StruggleJitterAppliedOffset = NewOffset;
}
