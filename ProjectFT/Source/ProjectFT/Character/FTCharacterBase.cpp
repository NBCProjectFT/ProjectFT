// Fill out your copyright notice in the Description page of Project Settings.

#include "FTCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "ProjectFT/AbilitySystem/FTAbilityTags.h"
#include "ProjectFT/AbilitySystem/FTAttributeSet.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_BubbleStackTrap.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGA_EscapableDebuff.h"
#include "ProjectFT/Struct/FTCharacterAttackedPayloadStruct.h"

AFTCharacterBase::AFTCharacterBase()
{
	// 발소리 거리 누적(UpdateFootstepDistance)이 매 프레임 갱신을 필요로 한다. ACharacter 기본값도 true지만,
	// 의존 관계를 드러내기 위해 명시한다 — 여기를 끄면 발소리가 조용히 사라진다.
	PrimaryActorTick.bCanEverTick = true;

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

void AFTCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFootstepDistance(DeltaSeconds);
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
	// AnimBP가 읽는 캐시이기도 하다(포즈 종류는 GetActiveImmobilizePoseTag가 그때그때 판정).
	bIsImmobilized = NewCount > 0;

	// 공통 반응: 행동불능이 되면 '진행 중이던' 아이템 동작도 끊는다. UFTGameplayAbility의 ActivationBlockedTags는
	// 새 발동만 막을 뿐 이미 도는 어빌리티엔 닿지 않아서, 행동불능 직전에 시작한 공격 몽타주가 계속 돌며 적중했다
	// (예: 잡히기 직전 휘두른 무기가 잡은 경비를 때려 그 자리에서 풀려나는 문제).
	// 취소 기준은 Ability.ItemUse 에셋 태그 — 행동불능 '중에' 돌아야 하는 탈출/트랩 어빌리티(UFTGA_EscapableDebuff,
	// UFTGA_BubbleStackTrap)는 아이템 동작이 아니라 여기 걸리지 않는다. 이동 정지보다 먼저 취소해야 어빌리티 종료가
	// 이동 모드를 되돌려놓아도 아래 봉쇄가 마지막 말이 된다.
	if (bIsImmobilized && AbilitySystemComponent)
	{
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(TAG_FT_Ability_ItemUse);
		AbilitySystemComponent->CancelAbilities(&CancelTags);
	}

	// 공통 반응: 행동불능 시작 시 현재 이동 모드를 저장하고 즉시 정지+이동 비활성, 해제 시 저장한 이동 모드로 복원.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (bIsImmobilized)
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
	OnImmobilizedStateChanged(bIsImmobilized);
}

void AFTCharacterBase::OnImmobilizedStateChanged(bool bImmobilized)
{
	// 기본 구현 없음. 자식이 AI 로직 정지/애니 등 추가 반응을 처리한다(이동 정지/복원은 베이스가 이미 처리).
}

FGameplayTag AFTCharacterBase::GetActiveImmobilizePoseTag() const
{
	if (!bIsImmobilized || !AbilitySystemComponent)
	{
		return FGameplayTag();
	}

	for (const FGameplayTag& Candidate : ImmobilizePosePriority)
	{
		if (Candidate.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(Candidate))
		{
			return Candidate;
		}
	}

	// 우선순위 목록이 비었거나 어느 것도 안 맞으면 우산 태그로 폴백한다.
	// 행동불능인데 빈 태그를 돌려주면 AnimBP가 포즈를 못 고르고 서 있게 되므로, '공용 행동불능' 포즈로 수렴시킨다.
	return TAG_FT_State_Debuff_Immobilized;
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

	// 피격 연출도 같은 신호에 얹는다 — 데미지/스턴/슬로우 구분 없이 "맞으면 반응"이 되도록.
	// 어그로 방송이 연출 실패에 영향받지 않게 방송 뒤에 재생한다.
	PlayHitReact(Payload.EffectTags);
}

void AFTCharacterBase::PlayHitReact(const FGameplayTagContainer& EffectTags)
{
	if (bDead)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 산탄/다단히트가 한 프레임에 여러 GE로 들어와도 몽타주가 처음부터 재시작하며 떨거나 피격음이 겹쳐 터지는 것을 막는다.
	// 소리와 몽타주가 같은 간격을 공유하므로 둘의 타이밍이 어긋나지 않는다.
	const float Now = World->GetTimeSeconds();
	if (HitReactMinInterval > 0.0f && (Now - LastHitReactTime) < HitReactMinInterval)
	{
		return;
	}

	// 소리와 몽타주 중 하나라도 실제로 나갔는지. 아무것도 못 냈으면(에셋 없음/재생 실패) 쿨다운을 찍지 않아
	// 다음 피격에서 다시 시도된다.
	bool bPlayedAnyFeedback = false;

	// 피격음은 구속(잡힘/비눗방울 등) 중에도 낸다 — 아래 몽타주와 달리 소리는 자세를 덮어쓰지 않으므로 막을 이유가 없다.
	// 오히려 잡혀서 몽타주가 봉쇄된 동안엔 소리가 유일한 피격 피드백이다.
	if (USoundBase* SoundToPlay = SelectHitReactSound(EffectTags))
	{
		// 캐릭터(루트)에 붙여 재생 — 맞고 밀려나거나 경비에게 끌려가는 중에도 소리가 몸을 따라간다.
		UGameplayStatics::SpawnSoundAttached(SoundToPlay, GetRootComponent());
		bPlayedAnyFeedback = true;
	}

	// 구속 연출이 도는 중엔 기본적으로 피격 몽타주로 상체를 덮어쓰지 않는다(자세가 풀려 보이므로).
	const bool bMontageAllowed = bPlayHitReactWhileImmobilized
		|| !AbilitySystemComponent
		|| !AbilitySystemComponent->HasMatchingGameplayTag(TAG_FT_State_Debuff_Immobilized);
	if (bMontageAllowed)
	{
		if (UAnimMontage* MontageToPlay = SelectHitReactMontage(EffectTags))
		{
			USkeletalMeshComponent* CharacterMesh = GetMesh();
			UAnimInstance* AnimInstance = CharacterMesh ? CharacterMesh->GetAnimInstance() : nullptr;

			// 재생 실패(슬롯 없음/에셋 불일치)는 성공으로 치지 않는다.
			if (AnimInstance && AnimInstance->Montage_Play(MontageToPlay, 1.0f) > 0.0f)
			{
				bPlayedAnyFeedback = true;
			}
		}
	}

	if (bPlayedAnyFeedback)
	{
		LastHitReactTime = Now;
	}
}

UAnimMontage* AFTCharacterBase::SelectHitReactMontage_Implementation(const FGameplayTagContainer& EffectTags) const
{
	// 기본은 공격 종류 불문 단일 몽타주. 태그별 분기는 BP/자식에서 override 한다.
	return HitReactMontage;
}

USoundBase* AFTCharacterBase::SelectHitReactSound_Implementation(const FGameplayTagContainer& EffectTags) const
{
	// 기본은 공격 종류 불문 단일 피격음. 태그별 분기(감전/화상 등)는 BP/자식에서 override 한다.
	return HitReactSound;
}

// ─────────────────────────────────────────────────────────────────────────────────────────
// [임시 구동부] 거리 누적 발소리 — 애님 노티파이로 전환하는 방법
//
// 왜 지금은 노티파이가 아닌가:
//   발소리 타이밍의 정석은 애님 노티파이다(접지 프레임에 정확히 붙고, 재생속도가 변해도 따라간다).
//   그런데 현재 '서서' 걷는 이동 블렌드가 시각적으로 깨져 있다 — 다리가 실제 이동속도보다 빠르게 돌아간다
//   (앉아서 이동은 정상). 노티파이는 그 어긋난 애니메이션을 그대로 반영하므로 소리까지 같이 어긋난다.
//   반면 거리 누적은 "이만큼 이동했으면 몇 보"라는 물리적 사실을 지켜 그 상태에서도 덜 어색하다.
//   유력한 원인은 UFTPlayerAnimInstance::UpdateCharacterState의 LocomotionPlayRate가 '현재 속도'가 아니라
//   최대 스프린트 속도로 계산돼 속도와 무관한 상수라는 점이다. 그쪽이 정리되면 아래 절차로 전환할 것.
//
// 전환 절차:
//   1) UFTFootstepAnimNotify(UAnimNotify 파생)를 Source/ProjectFT/AnimNotifies/에 추가한다.
//      선례는 같은 폴더의 FTThrowReleaseAnimNotify — 노티파이는 '신호'만 보내고 처리는 다른 곳이 한다.
//      구현은 MeshComp->GetOwner()를 AFTCharacterBase로 캐스트해 PlayFootstep()을 부르는 게 전부다.
//   2) 이동 '시퀀스'에 노티파이를 찍는다. 블렌드 스페이스 에셋(NEKO_BS_*)은 건드리지 않는다 —
//      노티파이는 블렌드 스페이스가 아니라 그것이 샘플링하는 시퀀스에 붙는다(NEKO_MF_Unarmed_Walk_*/Jog_*, 각 4방향).
//      접지 프레임을 새로 찾을 필요는 없다: 그 시퀀스들엔 이미 싱크 마커 LeftFootFX/RightFootFX가 찍혀 있으니
//      같은 프레임에 노티파이를 놓으면 된다. (NEKO_CrouchWalk엔 마커가 없어 수동으로 잡아야 한다.)
//   3) 블렌드 구간의 중복 발동은 따로 막을 필요가 없다. 블렌드 스페이스의 Notify Trigger Mode 기본값이
//      HighestWeightedAnimation이라, 섞이는 중에도 가중치가 가장 높은 샘플 하나에서만 노티파이가 나온다.
//   4) 걷어낼 것: 이 함수, Tick 오버라이드, FootstepStrideLength/FootstepMinSpeed/FootstepDistanceAccumulator,
//      그리고 생성자의 PrimaryActorTick.bCanEverTick(그때까지 다른 매 프레임 작업이 생기지 않았다면).
//      남길 것: PlayFootstep / SelectFootstepSound / FootstepSound — 호출 주체만 노티파이로 바뀐다.
//
// 주의: 노티파이에 사운드를 직접 박지 말 것.
//   살금걷기(NEKO_BS_Sneak_Walk)는 전용 시퀀스가 없어 일반 Walk 시퀀스를 샘플링할 가능성이 높다.
//   시퀀스에 소리가 박혀 있으면 몰래 걸을 때도 정상 보행과 같은 소리가 난다.
//   '언제'는 노티파이가, '무엇을'은 SelectFootstepSound가 정하는 분리를 유지해야 한다.
//   같은 이유로 바닥 재질별 발소리·AI 청각(MakeNoise)도 노티파이가 아니라 PlayFootstep 안에 들어가야 한다.
// ─────────────────────────────────────────────────────────────────────────────────────────
void AFTCharacterBase::UpdateFootstepDistance(float DeltaSeconds)
{
	// 죽었거나 행동불능(잡힘/스턴/비눗방울)이면 발소리 없음 — 끌려가는 중엔 스스로 걷는 게 아니다.
	if (bDead || bIsImmobilized)
	{
		FootstepDistanceAccumulator = 0.0f;
		return;
	}

	// 공중(점프/낙하)에선 발이 땅에 없으므로 누적하지 않는다. 착지음은 별도 연출의 몫.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || !Movement->IsMovingOnGround())
	{
		FootstepDistanceAccumulator = 0.0f;
		return;
	}

	// 위치 변화가 아니라 속도를 쓴다 — 순간이동이나 부착 이동(경비에게 끌려가는 등)으로 헛발소리가 나지 않는다.
	const float GroundSpeed = GetVelocity().Size2D();
	if (GroundSpeed < FootstepMinSpeed)
	{
		FootstepDistanceAccumulator = 0.0f;
		return;
	}

	FootstepDistanceAccumulator += GroundSpeed * DeltaSeconds;
	if (FootstepDistanceAccumulator < FootstepStrideLength)
	{
		return;
	}

	// 프레임이 크게 밀려 한 번에 여러 걸음치가 쌓여도 소리는 한 번만 낸다(겹쳐 들리기만 하므로).
	// 나머지 거리는 Fmod로 다음 걸음에 넘겨, 밀린 뒤에도 걸음 간격이 어긋나지 않는다.
	FootstepDistanceAccumulator = FMath::Fmod(FootstepDistanceAccumulator, FootstepStrideLength);
	PlayFootstep();
}

void AFTCharacterBase::PlayFootstep()
{
	if (bDead)
	{
		return;
	}

	USoundBase* SoundToPlay = SelectFootstepSound();
	if (!SoundToPlay)
	{
		return;
	}

	// 캐릭터(루트)에 붙여 재생 — 이동 중에도 소리가 몸을 따라간다(피격음과 동일한 방식).
	UGameplayStatics::SpawnSoundAttached(SoundToPlay, GetRootComponent());
}

USoundBase* AFTCharacterBase::SelectFootstepSound_Implementation() const
{
	// 기본은 상황 불문 단일 발소리. 앉기/달리기·바닥 재질별 분기는 BP/자식에서 override 한다.
	return FootstepSound;
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
