# LevelDataAssetGenerator

`LevelDataAssetGenerator`는 현재 Editor에 열려 있는 Level을 스캔해서 ProjectFT의 Level별 Preload DataAsset을 생성 또는 갱신하는 Editor Plugin입니다.

이 Plugin은 Runtime 로드를 직접 수행하지 않습니다. Runtime 로드는 `FTLoadingGameMode`, `FTGameFlowSubsystem`, `UFTAssetManager`가 담당합니다.

## ProjectFT 기준 역할

ProjectFT는 멀티플레이 권한에 따라 Asset 로드 범위를 나누지 않습니다. 하나의 Level에 진입하면 해당 Level에서 필요한 Asset은 전부 로드하는 쪽이 더 단순합니다.

따라서 이 Plugin은 Bundle을 만들지 않습니다.

역할은 하나입니다.

- 현재 Level에 배치된 환경 StaticMesh를 찾아 `UFTLevelPreloadDataAsset.GeneratedEnvironmentAssets`에 반영

사람이 직접 추가해야 하는 Level 전용 Asset은 `AdditionalPreloadAssets`에 넣습니다.

## 생성되는 DataAsset

ProjectFT는 `UFTLevelPreloadDataAsset`을 사용합니다.

```cpp
UCLASS(BlueprintType)
class PROJECTFT_API UFTLevelPreloadDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Level")
	FName LevelId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Level")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FT|Preload")
	TArray<TSoftObjectPtr<UObject>> GeneratedEnvironmentAssets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload")
	TArray<TSoftObjectPtr<UObject>> AdditionalPreloadAssets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FT|Preload")
	TArray<TSoftObjectPtr<UObject>> ExcludedAssets;
};
```

최종 Preload 대상은 다음과 같습니다.

```text
GeneratedEnvironmentAssets
+
AdditionalPreloadAssets
-
ExcludedAssets
```

## 기본 흐름

1. Editor에서 대상 Level을 엽니다.
2. `Tools > Generate Level Preload Data Asset`을 실행합니다.
3. 스캔된 StaticMesh 목록을 확인합니다.
4. Include를 조정합니다.
5. `Generate Level Preload Data Asset`을 누릅니다.
6. Plugin이 `DA_LevelPreload_<LevelName>` DataAsset의 `GeneratedEnvironmentAssets`를 갱신합니다.
7. `AdditionalPreloadAssets`와 `ExcludedAssets`는 사람이 DataAsset에서 직접 관리합니다.

## Runtime Loading

`UFTGameDataAsset`의 `FFTFlowStateDefinition`은 Level별 Preload DataAsset을 참조합니다.

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FT|Flow")
TSoftObjectPtr<UFTLevelPreloadDataAsset> LevelPreloadDataAsset;
```

로딩 흐름은 다음과 같습니다.

```text
TravelToState
-> Lvl_Loading
-> AFTLoadingGameMode::StartPlay
-> UFTGameFlowSubsystem::PreloadCurrentStateAssetsAsync
-> UFTAssetManager::PreloadLevelAssetsAsync
-> UFTLevelPreloadDataAsset::GetPreloadAssetPaths
-> 완료 후 TargetLevelName OpenLevel
```

## 설정

`Project Settings > Plugins > Level Data Asset Generator`에서 설정합니다.

- `GeneratedDataAssetClass`: 생성할 DataAsset Class. 기본값은 `UFTLevelPreloadDataAsset`
- `OutputFolder`: 생성 DataAsset 저장 경로
- `GeneratedAssetPrefix`: 생성 Asset 이름 접두사
- `LevelPropertyName`: DataAsset의 Level 프로퍼티 이름
- `LevelIdPropertyName`: DataAsset의 LevelId 프로퍼티 이름
- `GeneratedEnvironmentAssetsPropertyName`: 자동 생성 환경 Asset 배열 프로퍼티 이름
- `IgnoredPaths`: 스캔 제외 경로
- `IgnoreEngineAssets`: `/Engine` Asset 제외 여부
- `IgnorePluginAssets`: `/Game` 밖 Asset 제외 여부
- `Include StaticMeshComponent`: 일반 StaticMeshComponent 포함 여부
- `Include InstancedStaticMeshComponent`: InstancedStaticMeshComponent 포함 여부
- `Include HierarchicalInstancedStaticMeshComponent`: HISM 포함 여부

## 설계 원칙

- Bundle 개념을 사용하지 않습니다.
- Material, Texture 같은 하위 Dependency를 강제로 펼치지 않습니다.
- Plugin은 `GeneratedEnvironmentAssets`만 자동 갱신합니다.
- 수동 관리가 필요한 Asset은 `AdditionalPreloadAssets`에 둡니다.
- 제외가 필요한 Asset은 `ExcludedAssets`에 둡니다.
