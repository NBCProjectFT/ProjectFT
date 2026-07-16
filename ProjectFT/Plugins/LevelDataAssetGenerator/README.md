# LevelDataAssetGenerator

`LevelDataAssetGenerator`는 ProjectFT의 Level별 Preload DataAsset을 생성 또는 갱신하는 Editor Plugin입니다.

현재 구조에서는 Level에 이미 배치된 환경 StaticMesh를 Preload의 핵심 대상으로 보지 않습니다. 배치된 Mesh는 Level Load 과정에서 자연스럽게 로드되므로, 이 Plugin은 Level에서 런타임에 생성되거나 DataAsset/Soft Reference를 통해 사용되는 기능별 Asset을 수집합니다.

## 역할

- 현재 열린 Level의 이름을 기준으로 Preload Preset을 판별합니다.
- Preset에 켜진 기능별 Root Directory와 직접 지정 Asset을 수집합니다.
- AssetRegistry dependency를 통해 Runtime 의존 Asset을 확장합니다.
- 결과를 `UFTLevelPreloadDataAsset.GeneratedRuntimeAssets`에 저장합니다.
- Inventory Item 목록은 별도의 `UFTInventoryPreloadDataAsset`에 저장하고 Level DataAsset은 이를 참조합니다.

## 버튼

### Generate Level Preload Data Asset

현재 Editor에 열린 Level 기준으로 `DA_LevelPreload_<LevelName>`을 생성 또는 갱신합니다.

저장 대상:

```text
Level
LevelId
GeneratedRuntimeAssets
GeneratedEnvironmentAssets
bUseInventoryPreloadDataAsset
InventoryPreloadDataAsset
```

`GeneratedEnvironmentAssets`는 Legacy/옵션 필드입니다. `bIncludePlacedEnvironmentAssets`가 꺼져 있으면 비워집니다.

### Generate Inventory Item Preload Data Asset

설정된 Inventory Item DataAsset Directory를 스캔해서 공용 Inventory Preload DataAsset을 생성 또는 갱신합니다.

기본 대상:

```text
/Game/Blueprints/Items/Data
```

저장 대상:

```text
DA_InventoryPreload_Default.InventoryItemAssets
```

## 구조

Feature는 Preset 안에 직접 만들지 않습니다.

```text
Features
- 공통 Feature 목록
- 각 Feature는 FeatureName, RootDirectories, DirectAssets를 가짐

Presets
- Level별 조합 규칙
- 각 Preset은 FeatureNames로 공통 Feature를 선택함
```

즉 `Inventory`, `Combat`, `DamageText` 같은 Feature는 한 번만 정의하고, `Play`, `Hub`, `MainMenu` 같은 Preset은 필요한 Feature 이름만 선택합니다.

## Preset

Preset은 사용자가 직접 정의합니다.

Preset Definition:

```text
PresetName
- 사람이 직접 입력하는 Preset 이름

LevelIds
- 이 Preset을 사용할 Level 이름 목록
- 예: Lvl_Main, Market_Test

FeatureNames
- 이 Preset에서 사용할 Feature 이름 목록
- Features 배열에 있는 FeatureName과 일치해야 함

bAssignInventoryPreloadDataAsset
- 이 Preset의 LevelPreload DataAsset에 InventoryPreloadDataAsset을 연결할지 여부
```

설정 위치:

```text
Project Settings > Plugins > Level Data Asset Generator
```

기본 Preset 예시:

```text
Play
- LevelIds: Lvl_Main, Market_Test
- FeatureNames: Common, Inventory, Shelf, Combat, DamageText, AI, Quest

Hub
- LevelIds: Lvl_Hub
- FeatureNames: Common, Inventory, Crafting, Shop, Market, Storage, Quest

MainMenu
- LevelIds: Lvl_MainMenu
- FeatureNames: MainMenu
```

LevelId가 어떤 Preset에도 포함되지 않으면 `DefaultPresetName`에 해당하는 Preset을 사용합니다.

## 기능별 수집

Feature도 사용자가 직접 정의합니다. 이 목록은 모든 Preset이 공유하는 공통 목록입니다.

Feature Definition:

```text
FeatureName
- Preset에서 참조할 Feature 이름

RootDirectories
- 이 Feature가 수집할 Asset Root Directory 목록

DirectAssets
- Directory 검색 없이 직접 포함할 Asset 목록
```

즉, Feature Roots는 별도 고정 설정이 아니라 `Features` 배열의 각 Feature 내부에서 설정합니다.

예시:

```text
FeatureName: Inventory
RootDirectories:
- /Game/Blueprints/Items/Data
- /Game/UI/Inventory
- /Game/UI/HUD

FeatureName: DamageText
DirectAssets:
- /Game/UI/WBP_DamageText.WBP_DamageText
```

## 기본 Feature 예시

### Play용 Feature

```text
Inventory
Shelf
Combat
DamageText
AI
Quest
```

주요 대상:

```text
Hub UI
Crafting UI / Recipe Data
Shop UI / Shop Data
Market UI
Storage UI
Quest UI / Data
Inventory Item DataAsset
```

### Hub용 Feature

```text
Inventory
Crafting
Shop
Market
Storage
Quest
```

주요 대상:

```text
Hub UI
Crafting UI / Recipe Data
Shop UI / Shop Data
Market UI
Storage UI
Quest UI / Data
Inventory Item DataAsset
```

### MainMenu용 Feature

주요 대상:

```text
MainMenu UI
Menu Texture / Sound / Font
Flow 최소 데이터
```

## Dependency 확장

`bExpandRuntimeDependencies`가 켜져 있으면 Root Asset의 Hard/Soft Package Dependency를 따라가며 Runtime Asset을 추가합니다.

제외 대상:

```text
World
TextureRenderTarget2D
EditorUtilityWidgetBlueprint
EditorUtilityBlueprint
/Game/Blueprints/MakeIcons/Tool
/Developers
/Collections
/Editor
/Engine
Plugin Asset
IgnoredPaths
```

## Runtime Loading

최종 로드 대상은 `UFTLevelPreloadDataAsset::GetPreloadAssetPaths()`에서 계산됩니다.

```text
GeneratedEnvironmentAssets
+ GeneratedRuntimeAssets
+ AdditionalPreloadAssets
- ExcludedAssets
```

Inventory Item은 `InventoryPreloadDataAsset`을 통해 별도로 합류합니다.

## 설정 팁

- Level에 배치된 환경 Mesh까지 강제로 Preload하고 싶을 때만 `bIncludePlacedEnvironmentAssets`를 켭니다.
- 기능별 Root Directory가 너무 넓으면 Preload가 과해질 수 있습니다.
- 수집 결과에 Editor Tool Asset이 섞이면 `IgnoredPaths`에 추가합니다.
- 사람이 직접 예외로 넣어야 하는 Asset은 `AdditionalPreloadAssets`에 둡니다.
- 자동 수집에서 빠져야 하는 Asset은 `ExcludedAssets`에 둡니다.
