#include "FT_ItemDataParser.h"

#include "Engine/DataTable.h"
#include "ProjectFT/Struct/FTItemTableRowStruct.h"
#include "ProjectFT/AbilitySystem/Abilities/FTGameplayAbility.h"
#include "GameplayEffect.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "ProjectFT/Data/FTItemDataAsset.h"

namespace
{
	FString GetTrimmedValue(const TMap<FString, FString>& RowData, const TCHAR* Header)
	{
		return RowData.FindRef(Header).TrimStartAndEnd();
	}

	template<typename TEnum>
	TEnum ParseEnum(const FString& EnumName, const FString& StringValue)
	{
		UEnum* EnumPtr = FindObject<UEnum>(nullptr, *EnumName);
		if (!EnumPtr)
		{
			return TEnum(0);
		}
		int64 Value = EnumPtr->GetValueByName(FName(*StringValue));
		return (Value == INDEX_NONE) ? TEnum(0) : static_cast<TEnum>(Value);
	}

	void ParseEffectsList(const FString& Source, TArray<TSubclassOf<UGameplayEffect>>& OutValues, FString& OutError)
	{
		OutValues.Reset();
		TArray<FString> Tokens;
		Source.ParseIntoArray(Tokens, TEXT("|"), true);

		for (FString& Token : Tokens)
		{
			Token.TrimStartAndEndInline();
			if (Token.IsEmpty())
			{
				continue;
			}

			TSubclassOf<UGameplayEffect> EffectClass = LoadClass<UGameplayEffect>(nullptr, *Token);
			if (!EffectClass)
			{
				OutError = FString::Printf(TEXT("Failed to load GameplayEffect class: %s"), *Token);
				return;
			}
			OutValues.Add(EffectClass);
		}
	}

	bool ParseEffectMagnitudes(const FString& Source, TMap<FGameplayTag, float>& OutValues, FString& OutError)
	{
		OutValues.Reset();
		TArray<FString> Tokens;
		Source.ParseIntoArray(Tokens, TEXT("|"), true);

		for (FString& Token : Tokens)
		{
			Token.TrimStartAndEndInline();
			if (Token.IsEmpty())
			{
				continue;
			}

			FString TagString;
			FString ValString;
			if (!Token.Split(TEXT(":"), &TagString, &ValString, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
			{
				OutError = FString::Printf(TEXT("EffectMagnitude must use Tag:Value format: %s"), *Token);
				return false;
			}

			TagString.TrimStartAndEndInline();
			ValString.TrimStartAndEndInline();

			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
			if (!Tag.IsValid())
			{
				OutError = FString::Printf(TEXT("Invalid GameplayTag: %s"), *TagString);
				return false;
			}

			const float Val = FCString::Atof(*ValString);
			OutValues.Add(Tag, Val);
		}

		return true;
	}

	bool AreItemUsesEqual(const FTItemUseStruct& A, const FTItemUseStruct& B)
	{
		if (A.UseAbility != B.UseAbility) return false;
		if (A.UseEffects != B.UseEffects) return false;
		if (A.CastTimeSeconds != B.CastTimeSeconds) return false;
		if (A.CooldownSeconds != B.CooldownSeconds) return false;
		if (A.CooldownTag != B.CooldownTag) return false;

		if (A.EffectMagnitudes.Num() != B.EffectMagnitudes.Num()) return false;
		for (const auto& Pair : A.EffectMagnitudes)
		{
			const float* ValB = B.EffectMagnitudes.Find(Pair.Key);
			if (!ValB || *ValB != Pair.Value) return false;
		}

		return true;
	}

	bool AreItemDatasEqual(const FTItemDataStruct& A, const FTItemDataStruct& B)
	{
		if (A.ItemId != B.ItemId) return false;
		if (!A.ItemName.ToString().Equals(B.ItemName.ToString())) return false;
		if (!A.ItemDescription.ToString().Equals(B.ItemDescription.ToString())) return false;
		if (A.Weight != B.Weight) return false;
		if (A.Cost != B.Cost) return false;
		if (A.CategoryType != B.CategoryType) return false;
		if (A.WeaponStance != B.WeaponStance) return false;
		if (A.ItemIcon.ToSoftObjectPath() != B.ItemIcon.ToSoftObjectPath()) return false;
		if (A.ItemMesh != B.ItemMesh) return false;
		if (A.InventorySubstituteItem.ToSoftObjectPath() != B.InventorySubstituteItem.ToSoftObjectPath()) return false;
		if (!A.DropMeshScale.Equals(B.DropMeshScale)) return false;
		if (!A.IconMeshRotation.Equals(B.IconMeshRotation)) return false;
		if (!A.IconMeshLocationOffset.Equals(B.IconMeshLocationOffset)) return false;
		if (!A.IconCameraTargetOffset.Equals(B.IconCameraTargetOffset)) return false;
		if (A.IconCameraDistanceMultiplier != B.IconCameraDistanceMultiplier) return false;
		if (A.IconFOV != B.IconFOV) return false;

		if (!AreItemUsesEqual(A.UseData, B.UseData)) return false;

		return true;
	}
}

void UFT_ItemDataParser::OnParseComplete()
{
	UE_LOG(LogTemp, Log, TEXT("[ItemSheet] Parsed rows: %d"), GetRowCount());

	if (!IsValid(TargetTable))
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemSheet] TargetTable is empty."));
		return;
	}

	if (TargetTable->GetRowStruct() != FFTItemTableRowStruct::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemSheet] TargetTable row struct must be FFTItemTableRowStruct."));
		return;
	}

	const TArray<FString> RequiredHeaders = {
		TEXT("ItemID"), TEXT("ItemName"), TEXT("ItemDescription"), TEXT("Weight"), TEXT("Cost"),
		TEXT("CategoryType"), TEXT("WeaponStance"), TEXT("ItemIconPath"), TEXT("ItemMeshPath"),
		TEXT("InventorySubstituteItemPath"), TEXT("DropMeshScale"), TEXT("IconMeshRotation"),
		TEXT("IconMeshLocationOffset"), TEXT("IconCameraTargetOffset"), TEXT("IconCameraDistanceMultiplier"),
		TEXT("IconFOV"), TEXT("UseAbilityPath"), TEXT("UseEffectsPaths"), TEXT("EffectMagnitudes"),
		TEXT("CastTimeSeconds"), TEXT("CooldownSeconds"), TEXT("CooldownTag")
	};
	const TArray<FString> ActualHeaders = GetHeaders();
	for (const FString& Header : RequiredHeaders)
	{
		if (!ActualHeaders.Contains(Header))
		{
			UE_LOG(LogTemp, Error, TEXT("[ItemSheet] Missing required header: %s"), *Header);
			return;
		}
	}

	TMap<FName, FFTItemTableRowStruct> ParsedItems;
	bool bHasError = false;

	for (int32 RowIndex = 0; RowIndex < GetRowCount(); ++RowIndex)
	{
		TMap<FString, FString> RowData;
		if (!GetRowAt(RowIndex, RowData))
		{
			continue;
		}

		const FString ItemIDString = GetTrimmedValue(RowData, TEXT("ItemID"));
		if (ItemIDString.IsEmpty())
		{
			continue;
		}

		const FName ItemID(*ItemIDString);
		if (ParsedItems.Contains(ItemID))
		{
			UE_LOG(LogTemp, Error, TEXT("[ItemSheet] Duplicate ItemID at row %d: %s"), RowIndex + 2, *ItemIDString);
			bHasError = true;
			continue;
		}

		FFTItemTableRowStruct NewRow;
		FTItemDataStruct& Data = NewRow.ItemData;

		Data.ItemId = ItemID;
		Data.ItemName = FText::FromString(GetTrimmedValue(RowData, TEXT("ItemName")));
		Data.ItemDescription = FText::FromString(GetTrimmedValue(RowData, TEXT("ItemDescription")));

		const FString WeightString = GetTrimmedValue(RowData, TEXT("Weight"));
		Data.Weight = WeightString.IsEmpty() ? 0.0f : FCString::Atof(*WeightString);

		const FString CostString = GetTrimmedValue(RowData, TEXT("Cost"));
		Data.Cost = CostString.IsEmpty() ? 0 : FCString::Atoi(*CostString);

		Data.CategoryType = ParseEnum<EFTItemCategoryType>(TEXT("/Script/ProjectFT.EFTItemCategoryType"), GetTrimmedValue(RowData, TEXT("CategoryType")));
		Data.WeaponStance = ParseEnum<EFTWeaponStanceType>(TEXT("/Script/ProjectFT.EFTWeaponStanceType"), GetTrimmedValue(RowData, TEXT("WeaponStance")));

		const FString IconPath = GetTrimmedValue(RowData, TEXT("ItemIconPath"));
		if (!IconPath.IsEmpty())
		{
			Data.ItemIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath));
		}

		const FString MeshPath = GetTrimmedValue(RowData, TEXT("ItemMeshPath"));
		if (!MeshPath.IsEmpty())
		{
			Data.ItemMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
			if (!Data.ItemMesh)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ItemSheet] Row %d (%s): Failed to load StaticMesh: %s"), RowIndex + 2, *ItemIDString, *MeshPath);
			}
		}

		const FString SubstitutePath = GetTrimmedValue(RowData, TEXT("InventorySubstituteItemPath"));
		if (!SubstitutePath.IsEmpty())
		{
			Data.InventorySubstituteItem = TSoftObjectPtr<UFTItemDataAsset>(FSoftObjectPath(SubstitutePath));
		}

		const FString ScaleStr = GetTrimmedValue(RowData, TEXT("DropMeshScale"));
		Data.DropMeshScale = ScaleStr.IsEmpty() ? FVector(1.0f, 1.0f, 1.0f) : ParseToVector(ScaleStr);

		const FString RotStr = GetTrimmedValue(RowData, TEXT("IconMeshRotation"));
		if (!RotStr.IsEmpty())
		{
			FVector V = ParseToVector(RotStr);
			Data.IconMeshRotation = FRotator(V.X, V.Y, V.Z);
		}

		const FString LocOffsetStr = GetTrimmedValue(RowData, TEXT("IconMeshLocationOffset"));
		Data.IconMeshLocationOffset = LocOffsetStr.IsEmpty() ? FVector::ZeroVector : ParseToVector(LocOffsetStr);

		const FString CamTargetOffsetStr = GetTrimmedValue(RowData, TEXT("IconCameraTargetOffset"));
		Data.IconCameraTargetOffset = CamTargetOffsetStr.IsEmpty() ? FVector::ZeroVector : ParseToVector(CamTargetOffsetStr);

		const FString DistMulStr = GetTrimmedValue(RowData, TEXT("IconCameraDistanceMultiplier"));
		Data.IconCameraDistanceMultiplier = DistMulStr.IsEmpty() ? 2.8f : FCString::Atof(*DistMulStr);

		const FString FOVStr = GetTrimmedValue(RowData, TEXT("IconFOV"));
		Data.IconFOV = FOVStr.IsEmpty() ? 28.0f : FCString::Atof(*FOVStr);

		// UseData Parsing
		const FString UseAbilityPath = GetTrimmedValue(RowData, TEXT("UseAbilityPath"));
		if (!UseAbilityPath.IsEmpty())
		{
			Data.UseData.UseAbility = LoadClass<UFTGameplayAbility>(nullptr, *UseAbilityPath);
			if (!Data.UseData.UseAbility)
			{
				UE_LOG(LogTemp, Error, TEXT("[ItemSheet] Row %d (%s): Failed to load UseAbility class: %s"), RowIndex + 2, *ItemIDString, *UseAbilityPath);
				bHasError = true;
				continue;
			}
		}

		const FString UseEffectsPaths = GetTrimmedValue(RowData, TEXT("UseEffectsPaths"));
		if (!UseEffectsPaths.IsEmpty())
		{
			FString EffectsError;
			ParseEffectsList(UseEffectsPaths, Data.UseData.UseEffects, EffectsError);
			if (!EffectsError.IsEmpty())
			{
				UE_LOG(LogTemp, Error, TEXT("[ItemSheet] Row %d (%s): %s"), RowIndex + 2, *ItemIDString, *EffectsError);
				bHasError = true;
				continue;
			}
		}

		const FString EffectMagnitudesStr = GetTrimmedValue(RowData, TEXT("EffectMagnitudes"));
		if (!EffectMagnitudesStr.IsEmpty())
		{
			FString MagsError;
			if (!ParseEffectMagnitudes(EffectMagnitudesStr, Data.UseData.EffectMagnitudes, MagsError))
			{
				UE_LOG(LogTemp, Error, TEXT("[ItemSheet] Row %d (%s): %s"), RowIndex + 2, *ItemIDString, *MagsError);
				bHasError = true;
				continue;
			}
		}

		const FString CastTimeStr = GetTrimmedValue(RowData, TEXT("CastTimeSeconds"));
		Data.UseData.CastTimeSeconds = CastTimeStr.IsEmpty() ? 0.0f : FCString::Atof(*CastTimeStr);

		const FString CooldownStr = GetTrimmedValue(RowData, TEXT("CooldownSeconds"));
		Data.UseData.CooldownSeconds = CooldownStr.IsEmpty() ? 0.0f : FCString::Atof(*CooldownStr);

		const FString CooldownTagStr = GetTrimmedValue(RowData, TEXT("CooldownTag"));
		if (!CooldownTagStr.IsEmpty())
		{
			Data.UseData.CooldownTag = FGameplayTag::RequestGameplayTag(FName(*CooldownTagStr), false);
		}

		ParsedItems.Add(ItemID, MoveTemp(NewRow));
	}

	if (bHasError || ParsedItems.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemSheet] Import aborted. The existing DataTable was not changed."));
		return;
	}

	TargetTable->Modify();
	TargetTable->EmptyTable();
	for (const TPair<FName, FFTItemTableRowStruct>& Pair : ParsedItems)
	{
		TargetTable->AddRow(Pair.Key, Pair.Value);
	}
	TargetTable->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[ItemSheet] Imported %d items into %s."), ParsedItems.Num(), *TargetTable->GetName());

	// ── 개별 데이터 에셋 자동 동기화 ──
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;

	FTopLevelAssetPath ClassPath = UFTItemDataAsset::StaticClass()->GetClassPathName();
	FARFilter Filter;
	Filter.ClassPaths.Add(ClassPath);
	Filter.bRecursiveClasses = true; // 자식 클래스(UFTProjectileActorDataAsset, UFTMeleeDataAsset 등)도 포함
	
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	int32 UpdatedAssetCount = 0;
	for (const FAssetData& AssetData : AssetDataList)
	{
		UFTItemDataAsset* ItemAsset = Cast<UFTItemDataAsset>(AssetData.GetAsset());
		if (ItemAsset)
		{
			const FName ID = ItemAsset->ItemData.ItemId;
			if (ParsedItems.Contains(ID))
			{
				if (!AreItemDatasEqual(ItemAsset->ItemData, ParsedItems[ID].ItemData))
				{
					ItemAsset->Modify();
					ItemAsset->ItemData = ParsedItems[ID].ItemData;
					ItemAsset->MarkPackageDirty();
					UpdatedAssetCount++;
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ItemSheet] Automatically synchronized %d individual ItemDataAssets from Sheet."), UpdatedAssetCount);
}
