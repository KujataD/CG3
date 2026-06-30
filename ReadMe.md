# KujakuEngine

KujakuEngine は、DirectX 12 の描画基盤に、ImGui Docking を使った簡易エディタ、Unity 風の `Scene` / `GameObject` / `Component` 構造、JSON による Scene 保存・復元、Project Window、Transform Gizmo を追加した学習・制作向けの C++ エンジンです。

現在の設計では、`main.cpp` はできるだけ薄くし、Editor / Play の制御は `EditorApplication`、ゲーム内の Object 管理は `Scene`、個別の振る舞いは `Component` に寄せています。

## 目次

- [必要環境](#必要環境)
- [起動方法](#起動方法)
- [全体構造](#全体構造)
- [実行フロー](#実行フロー)
- [Editor の使い方](#editor-の使い方)
- [Play / Edit の仕様](#play--edit-の仕様)
- [Scene / GameObject / Component](#scene--gameobject--component)
- [標準 Component](#標準-component)
- [Scene JSON 保存と復元](#scene-json-保存と復元)
- [Game Window 選択と Transform Gizmo](#game-window-選択と-transform-gizmo)
- [Camera / Light の扱い](#camera--light-の扱い)
- [Model / Texture / 描画](#model--texture--描画)
- [Project Window](#project-window)
- [新しい Component の作り方](#新しい-component-の作り方)
- [新しい Scene の作り方](#新しい-scene-の作り方)
- [主要ファイル一覧](#主要ファイル一覧)
- [実装時の注意](#実装時の注意)
- [よくある問題](#よくある問題)

## 必要環境

- Windows
- Visual Studio 2022 以降
- C++20 相当のコンパイラ
- DirectX 12
- x64 / Debug または x64 / Release

外部ライブラリは `DirectXGame/externals` 配下に置かれています。

- Dear ImGui docking
- ImGuizmo
- DirectXTex
- nlohmann/json
- Assimp

Assimp を使うモデル読み込みでは、プロジェクト側と Assimp 側の Runtime Library 設定を揃える必要があります。Debug で静的ランタイムを使うなら `/MTd`、Release で静的ランタイムを使うなら `/MT` に揃えます。

## 起動方法

1. `KujakuEngine.sln` を Visual Studio で開きます。
2. 構成を `Debug`、プラットフォームを `x64` にします。
3. スタートアッププロジェクトを `DirectXGame` にします。
4. 実行します。

エントリーポイントは `DirectXGame/main.cpp` です。

```cpp
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	KujakuEngine::Initialize(L"LC2B_04_オオツカ_ダイチ_AL3");

	EditorApplication* editorApplication = EditorApplication::GetInstance();
	editorApplication->Initialize();
	editorApplication->SetCurrentScene(std::make_unique<SampleScene>());

	while (KujakuEngine::Update()) {
		editorApplication->BeginFrame();
		editorApplication->Update();
		editorApplication->Draw();
		editorApplication->EndFrame();
	}

	editorApplication->Finalize();
	KujakuEngine::Finalize();
	return 0;
}
```

`main.cpp` は `EditorApplication` を呼ぶだけに近い形になっています。Edit / Play の分岐や、Scene の Update を流すかどうかの判断は `main.cpp` には置かない方針です。

## 全体構造

```mermaid
graph TD
	main["DirectXGame/main.cpp"] --> engine["KujakuEngine::Initialize / Update / Finalize"]
	main --> editorApp["EditorApplication"]

	editorApp --> imgui["ImGuiManager"]
	editorApp --> scene["Scene"]
	editorApp --> jsonImport["SceneJsonImporter"]

	imgui --> gameWindow["Game Window"]
	imgui --> hierarchy["Hierarchy"]
	imgui --> inspector["Inspector"]
	imgui --> project["Project Window"]
	imgui --> console["Console"]

	scene --> gameObject["GameObject"]
	gameObject --> transform["TransformComponent"]
	gameObject --> component["Component"]
	component --> renderer["ModelRendererComponent"]
	component --> camera["CameraComponent / DebugCameraComponent"]
	component --> light["DirectionalLightComponent / PointLightComponent"]

	scene --> modelDraw["Model / Sprite / Particle Draw"]
	modelDraw --> dx["DirectXCommon / GraphicsPipeline"]
```

責務は次のように分けています。

| 層 | 主な役割 |
| --- | --- |
| `main.cpp` | エンジン初期化、EditorApplication 呼び出し、終了処理 |
| `KujakuEngine` | WinApp、DirectXCommon、Input、Time、TextureManager などの初期化と毎フレーム更新 |
| `EditorApplication` | EditorMode、Start / Stop、Scene 所有、Play 中だけ Scene Update を流す判断 |
| `ImGuiManager` | ImGui 初期化、DockSpace、各 Editor Window、ショートカット、Gizmo、Game Window 表示 |
| `Scene` | GameObject 一覧の所有、Update / Draw / Finalize の入口 |
| `GameObject` | 名前、Active、必須 Transform、Component 一覧 |
| `Component` | GameObject に追加する振る舞い、Inspector、JSON 保存・復元 |
| `DirectXCommon` | DX12 Device、SwapChain、Command、Descriptor、Game 用 RenderTarget |

## 実行フロー

### 起動時

1. `KujakuEngine::Initialize()` が呼ばれます。
2. `WinApp` がウィンドウを作成します。
3. `DirectXCommon` が DX12、SwapChain、Game 用 RenderTarget を初期化します。
4. `ImGuiManager` が ImGui を初期化します。
5. `GraphicsPipeline`、Light、TextureManager、Input、Random、Time などを初期化します。
6. `EditorApplication::Initialize()` が呼ばれ、標準 Component を登録し、EditorMode を `Edit` にします。
7. `EditorApplication::SetCurrentScene(std::make_unique<SampleScene>())` で Scene をセットします。
8. `SampleScene::Initialize()` で初期 GameObject を作ります。
9. `SceneJsonImporter` が `SceneJson` を探し、保存済み JSON があれば Scene に適用します。

### 毎フレーム

```cpp
while (KujakuEngine::Update()) {
	editorApplication->BeginFrame();
	editorApplication->Update();
	editorApplication->Draw();
	editorApplication->EndFrame();
}
```

各関数の中身は次のような役割です。

| 関数 | 役割 |
| --- | --- |
| `KujakuEngine::Update()` | Windows メッセージ、Input、Time を更新 |
| `EditorApplication::BeginFrame()` | ImGui のフレーム開始 |
| `EditorApplication::Update()` | Editor UI を描画し、Play 中だけ `currentScene_->Update()` |
| `EditorApplication::Draw()` | Game 用 RenderTarget に Scene を描画 |
| `EditorApplication::EndFrame()` | ImGui 描画、SwapChain Present、Fence 待機 |

## Editor の使い方

起動すると ImGui Docking ベースの Editor が表示されます。

| Window | 役割 |
| --- | --- |
| `Game` | ゲーム描画結果を表示します。Edit 中は DebugCamera、Play 中は Main Camera を使います。 |
| `Hierarchy` | Scene 内の GameObject 一覧を表示します。選択状態は Inspector と同期します。 |
| `Inspector` | 選択中 GameObject の名前、Active、Component を編集します。 |
| `Project` | `DirectXGame` 配下のファイルを閲覧します。画像・モデルはプレビューできます。 |
| `Console` | Editor の Start / Stop、JSON 保存・読込などのログを表示します。 |

### 基本操作

| 操作 | 内容 |
| --- | --- |
| `Start` ボタン | Edit から Play に移行します。 |
| `Stop` ボタン | Play から Edit に戻ります。 |
| `Ctrl + S` | 現在の Scene を JSON として保存します。 |
| Hierarchy の左クリック | GameObject を選択します。 |
| Game Window の左クリック | Edit 中だけ、描画されている GameObject を選択します。 |
| Hierarchy の右クリック | `Entity`、`Cube`、`Sphere` を作成できます。 |
| Inspector の `Add Component` | 登録済み Component を追加できます。 |
| Project Window の右クリック | `Copy File Path`、`Show in Explorer` を実行できます。 |

## Play / Edit の仕様

EditorMode は `EditorApplication` が管理します。

```cpp
enum class EditorMode {
	Edit,
	Play,
};
```

現在の仕様は次の通りです。

| 状態 | Scene Update | Scene Draw | Camera |
| --- | --- | --- | --- |
| Edit | 呼ばない | 呼ぶ | Editor Debug Camera |
| Play | 呼ぶ | 呼ぶ | Main Camera |

`EditorApplication::Update()` では次の考え方で制御しています。

```cpp
void EditorApplication::Update() {
	ImGuiManager::GetInstance()->DrawEditor();

	if (ShouldUpdateGame() && currentScene_) {
		currentScene_->Update();
	}
}
```

`ShouldUpdateGame()` は現在 `IsPlaying()` を返します。将来的に Scene Clone を実装する場合も、この入口を拡張する想定です。

### Start / Stop

`Start()` は Play に入り、Scene と GameObject と Component の `OnPlayStart()` を呼びます。

`Stop()` は Edit に戻り、`OnPlayStop()` を呼びます。

`TransformSnapshotComponent` を付けた GameObject は、Play 開始時の Transform を保存し、Stop 時に復元できます。ただし、現時点では Unity のような完全な PlayScene Clone はまだ実装していません。

## Scene / GameObject / Component

### Scene

`Scene` は GameObject 一覧を所有し、Update / Draw の入口になります。

```cpp
class Scene {
public:
	virtual void Initialize();
	virtual void Update();
	virtual void Draw();
	virtual void Finalize();

	GameObject* CreateGameObject(const std::string& name = "GameObject");
	GameObject* AddGameObject(std::unique_ptr<GameObject> gameObject);

	std::vector<std::unique_ptr<GameObject>>& GetGameObjects();
};
```

`main.cpp` や `EditorApplication` が個別の Player、Enemy、Model を直接 Update / Draw するのではなく、外側は Scene の入口だけを呼びます。

```cpp
if (ShouldUpdateGame() && currentScene_) {
	currentScene_->Update();
}

if (currentScene_) {
	currentScene_->Draw();
}
```

### GameObject

`GameObject` は Scene 上に存在する Object の最小単位です。

主な情報は次の通りです。

| 情報 | 内容 |
| --- | --- |
| `name_` | Hierarchy / Inspector に表示する名前 |
| `active_` | 無効化された GameObject は Update / Draw 対象外 |
| `TransformComponent` | 必須 Component。削除不可 |
| `components_` | `std::vector<std::unique_ptr<Component>>` で所有する Component 一覧 |

`GameObject` は必ず `TransformComponent` を持ちます。Inspector 上でも `Transform` は他の Component と同じ階層で表示されます。

```text
Transform
ModelRendererComponent
RotatorComponent
```

### Component

`Component` は GameObject に追加する振る舞いの基底クラスです。

```cpp
class Component {
public:
	virtual void Initialize() {}
	virtual void Update() {}
	virtual void Draw() {}
	virtual const char* GetTypeName() const { return "Component"; }
	virtual void DrawInspector() {}
	virtual void WriteJson(nlohmann::json& json) const {}
	virtual void ReadJson(const nlohmann::json& json) {}
	virtual void OnAfterReadJson() {}
	virtual void OnPlayStart() {}
	virtual void OnPlayStop() {}

	virtual bool CanRemove() const { return true; }
	virtual bool AllowMultiple() const { return true; }
};
```

Component は次の目的を持ちます。

- `Update()` で Play 中の挙動を作る。
- `Draw()` で描画処理を行う。
- `DrawInspector()` で Inspector の編集 UI を出す。
- `WriteJson()` / `ReadJson()` で保存・復元する。
- `OnPlayStart()` / `OnPlayStop()` で Play 開始・終了時の処理を行う。

## 標準 Component

現在登録されている標準 Component は `DirectXGame/KujakuEngine/components/BuiltinComponents.cpp` で登録されています。

| Component | 役割 |
| --- | --- |
| `Transform` | GameObject の位置、回転、スケールを持つ必須 Component。削除不可、複数追加不可。 |
| `ModelRendererComponent` | `Model` を GameObject の Transform で描画します。Cube / Sphere / Model / Custom を扱います。 |
| `CameraComponent` | GameObject の Transform を Camera に反映します。 |
| `DebugCameraComponent` | Edit 中の DebugCamera 操作を GameObject の Transform と Camera に反映します。 |
| `DirectionalLightComponent` | DirectionalLight の GPU データを GameObject 上で管理します。 |
| `PointLightComponent` | GameObject の Transform 位置を PointLight に反映します。 |
| `RotatorComponent` | 所有 GameObject を回転させるサンプル Component です。 |
| `TransformSnapshotComponent` | Play 開始時の Transform を保存し、Stop 時に戻します。 |

Inspector の `Add Component` メニューには、`ComponentFactory` に登録された Component 名が表示されます。

## Scene JSON 保存と復元

### 保存操作

`Ctrl + S` で現在の Scene を JSON として保存します。Export 専用ボタンは使わず、Editor ショートカットとして処理しています。

実装場所は `ImGuiManager::HandleEditorShortcuts()` と `ImGuiManager::ExportCurrentSceneJson()` です。

```cpp
if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
	ExportCurrentSceneJson();
}
```

### 保存先

ProjectDir は通常 `DirectXGame` です。保存先は次の構造になります。

```text
DirectXGame/
  SceneJson/
    SampleScene/
      SampleScene.scene.json
      GameObjects/
        0000_Main Camera.gameobject.json
        0001_Editor Debug Camera.gameobject.json
        0002_Directional Light.gameobject.json
        0003_Point Light.gameobject.json
        0004_MonsterBall.gameobject.json
        ...
```

`.meta` ファイルは現在生成しません。以前は Unity の meta 風ファイルを検討していましたが、現状のエンジンでは参照解決に使っていないため、生成処理は削除しています。

### Scene JSON

Scene 側の JSON は、GameObject JSON への参照を持ちます。

```json
{
  "assetType": "Scene",
  "name": "SampleScene",
  "gameObjects": [
    {
      "name": "MonsterBall",
      "assetPath": "SceneJson/SampleScene/GameObjects/0004_MonsterBall.gameobject.json"
    }
  ]
}
```

### GameObject JSON

GameObject 側の JSON は、名前、Active、Component 一覧を持ちます。

```json
{
  "assetType": "GameObject",
  "name": "MonsterBall",
  "active": true,
  "components": [
    {
      "type": "Transform",
      "enabled": true,
      "properties": {
        "scale": [1.0, 1.0, 1.0],
        "rotation": [0.0, 0.0, 0.0],
        "translation": [0.0, -2.0, 0.0]
      }
    },
    {
      "type": "ModelRendererComponent",
      "enabled": true,
      "properties": {
        "primitive": "Sphere",
        "texture": "resources/monsterBall.png",
        "modelPath": "player"
      }
    }
  ]
}
```

### 起動時の復元

`EditorApplication::SetCurrentScene()` の中で Scene 初期化後に `SceneJsonImporter::ImportScene()` を呼びます。

```cpp
currentScene_ = std::move(scene);
if (currentScene_) {
	currentScene_->Initialize();
	SceneJsonImporter::ImportScene(*currentScene_, DetectEditorProjectRoot());
}
```

保存済み JSON が存在する場合は、起動時にその内容が Scene へ適用されます。

Import の特徴は次の通りです。

- Scene 名から `SceneJson/<SceneName>/<SceneName>.scene.json` を探します。
- GameObject はまず名前一致で既存 Object に適用します。
- 一致しない場合は index を見ます。
- それでも存在しない場合は新規 GameObject を作ります。
- JSON にない削除可能 Component は削除されます。
- JSON にない GameObject は削除されます。
- Transform は必須 Component として扱い、削除しません。
- Component は `ComponentFactory` に登録された型名から生成されます。

## Game Window 選択と Transform Gizmo

### Game Window のアスペクト比

Game Window は Game 用 RenderTarget を `ImGui::Image` で表示しています。表示領域の縦横比が RenderTarget と合わない場合は、黒帯を描画して中央に表示します。

```cpp
drawList->AddRectFilled(contentPosition, contentEnd, IM_COL32(0, 0, 0, 255));
drawList->AddImage(static_cast<ImTextureID>(handle.ptr), imagePosition, imageEnd);
```

マウス座標や Gizmo の矩形も、黒帯を除いた実際の `imagePosition` / `imageSize` を基準に計算しています。そのため、ウィンドウ最大化時でも Game Window 上の判定がずれにくい構造です。

### GameObject のクリック選択

Edit 中に Game Window を左クリックすると、描画中の GameObject を選択できます。

Editor 側の入口は `ImGuiManager::HandleGameWindowObjectSelection()` です。実際の Ray 作成と Scene へのヒット判定は `scene/RayCast.h/.cpp` に分離しているため、ゲーム中の処理からも同じ仕組みを使えます。

```cpp
Ray ray{};
if (RayCast::CreateRayFromViewportPoint(point, viewportPosition, viewportSize, camera, ray)) {
	RayCastHit hit{};
	if (RayCast::Cast(scene, ray, hit)) {
		GameObject* selected = hit.gameObject;
	}
}
```

処理の流れは次の通りです。

1. Play 中なら選択しません。
2. Game Window 上のクリックか確認します。
3. ImGuizmo 操作中なら選択しません。
4. `RayCast::CreateRayFromViewportPoint()` でマウス座標を Camera 基準の Ray に変換します。
5. `RayCast::Cast()` で Scene 内の GameObject を走査します。
6. Active な `GameObject` だけを対象にします。
7. `ModelRendererComponent` を持つ Object だけを対象にします。
8. Model の頂点からローカル AABB を作ります。
9. `Model::GetRootLocalMatrix()` と GameObject Transform を合わせて World 行列を作ります。
10. Ray と AABB の交差判定を行い、最も近い Object を `RayCastHit` として返します。

選択状態は `EditorSelection` が持ちます。Hierarchy と Inspector は同じ `EditorSelection` を見ているため、Game Window で選んだ Object は Hierarchy 側でも選択中に見えます。

### Transform Gizmo

Transform Gizmo は ImGuizmo を使っています。

| モード | アイコン |
| --- | --- |
| Translate | `resources/images/icon_guizmo_translate.png` |
| Rotate | `resources/images/icon_guizmo_rotate.png` |
| Scale | `resources/images/icon_guizmo_scale.png` |

`DrawTransformGizmo()` は選択中 GameObject の `TransformComponent` を取得し、ImGuizmo の操作結果を `WorldTransform` の `translation_`、`rotation_`、`scale_` に戻します。

```cpp
WorldTransform& transform = transformComponent->GetTransform();
Matrix4x4 gizmoMatrix = MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);

ImGuizmo::Manipulate(
	MatrixData(camera->matView),
	MatrixData(camera->matProjection),
	operation,
	ImGuizmo::LOCAL,
	MatrixData(gizmoMatrix));
```

Scale は 0 に近づきすぎると描画や行列計算が壊れやすいため、最小値を `0.001f` に補正しています。

## Camera / Light の扱い

Camera、DebugCamera、DirectionalLight、PointLight は GameObject + Component として扱います。

`SampleScene::EnsureSceneServiceObjects()` が、標準サービス Object を存在確認し、なければ作成します。

| GameObject | Component | 役割 |
| --- | --- | --- |
| `Main Camera` | `CameraComponent` | Play 中の表示 Camera |
| `Editor Debug Camera` | `CameraComponent` + `DebugCameraComponent` | Edit 中の表示 Camera |
| `Directional Light` | `DirectionalLightComponent` | 平行光源 |
| `Point Light` | `PointLightComponent` | 点光源 |

Edit 中は `Editor Debug Camera` を使い、Play 中は `Main Camera` を使います。

```cpp
if (EditorApplication::GetInstance()->IsPlaying()) {
	currentViewCameraComponent_ = gameCameraComponent_;
} else {
	editorDebugCameraComponent_->UpdateEditorCamera();
	currentViewCameraComponent_ = editorCameraComponent_;
}
```

Light は `SampleScene::ApplySceneLights()` で Scene 内 Component を走査し、GPU 側の Light 管理クラスへ反映します。

## Model / Texture / 描画

### Model

`Model` は 3D モデルの頂点バッファ、マテリアル、テクスチャ、描画処理を持ちます。

主な生成関数は次の通りです。

```cpp
Model* model = Model::CreateFromOBJ("player", ShaderModel::kBlingPhongReflection);
Model* gltf = Model::CreateFromGlTF("plane", ShaderModel::kBlingPhongReflection);
Model* cube = Model::CreateCube("resources/monsterBall.png", ShaderModel::kBlingPhongReflection);
Model* sphere = Model::CreateSphere("resources/monsterBall.png", ShaderModel::kBlingPhongReflection);
```

`ModelUtil` は Assimp を使って OBJ / glTF などを読み込み、エンジン内部の `ModelData` へ変換します。Assimp 読み込み時は三角形化を前提にしており、四角形や多角形を含むモデルでも三角形として扱いやすくします。

`Model::GetRootLocalMatrix()` は Assimp の RootNode 行列を保持するために使います。描画や Game Window のクリック選択では、GameObject の Transform だけでなく RootNode のローカル行列も考慮します。

### ModelRendererComponent

`ModelRendererComponent` は GameObject の Transform で `Model` を描画する Component です。

Scene 内で描画用 Camera を取得できる場合は、次のように追加します。

```cpp
Camera* renderCamera = GetEditorCamera();

GameObject* cube = CreateGameObject("Cube");
ModelRendererComponent* renderer = cube->AddComponent<ModelRendererComponent>(renderCamera);
renderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Cube, "resources/monsterBall.png");
```

Inspector では次の項目を編集できます。

- `Primitive`: `Custom` / `Cube` / `Sphere` / `Model`
- `Texture`: テクスチャパス
- `Model`: モデルフォルダ名
- `Apply`: 設定を反映

現状の Editor 用途では、主に `Cube` / `Sphere` と Texture 変更が安定して使えます。OBJ / glTF の Editor 上での本格的な選択・永続化は今後拡張予定です。

### TextureManager

`TextureManager` はテクスチャ読み込みと SRV 管理を担当します。

```cpp
uint32_t textureIndex = TextureManager::GetInstance()->LoadTexture("resources/monsterBall.png");
D3D12_GPU_DESCRIPTOR_HANDLE handle = TextureManager::GetInstance()->GetSrvHandle(textureIndex);
```

Project Window では任意ファイルを扱うため、失敗しても assert しない `TryLoadTexture()` を使っています。

## Project Window

`ProjectWindow` は Unity の Project Window に近いファイル閲覧 UI です。

Project root は `EditorProjectPath::DetectEditorProjectRoot()` で検出します。通常は `DirectXGame` が root になります。

表示対象は ProjectDir 配下に制限されます。ProjectDir 外へ移動しないようにガードしています。

### アセット分類

`ProjectAssetClassifier` が拡張子やフォルダ状態から表示方法を決めます。

| 種類 | 表示 |
| --- | --- |
| Folder | 空 / 非空フォルダアイコン |
| Image | 実画像プレビュー |
| Model | 小さな RenderTarget に描いたモデルプレビュー |
| Audio | 音声ファイルアイコン |
| Other | 通常ファイルアイコン |

### 右クリックメニュー

Project Window のファイルを右クリックすると、次のメニューが出ます。

| メニュー | 内容 |
| --- | --- |
| `Copy File Path` | 絶対パスをクリップボードにコピーします。 |
| `Show in Explorer` | Explorer で対象ファイルを選択表示します。 |

## 新しい Component の作り方

ここでは、GameObject を Z 方向に動かす Component を例にします。

### 1. ヘッダを作る

`DirectXGame/KujakuEngine/components/MoveForwardComponent.h`

```cpp
#pragma once

#include "../scene/Component.h"

namespace KujakuEngine {

class MoveForwardComponent : public Component {
public:
	const char* GetTypeName() const override { return "MoveForwardComponent"; }

	void Update() override;
	void DrawInspector() override;
	void WriteJson(nlohmann::json& json) const override;
	void ReadJson(const nlohmann::json& json) override;

private:
	float speed_ = 0.05f;
};

} // namespace KujakuEngine
```

### 2. cpp を作る

`DirectXGame/KujakuEngine/components/MoveForwardComponent.cpp`

```cpp
#include "MoveForwardComponent.h"
#include "../scene/GameObject.h"
#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif

namespace KujakuEngine {

void MoveForwardComponent::Update() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().translation_.z += speed_;
}

void MoveForwardComponent::DrawInspector() {
#ifdef USE_IMGUI
	ImGui::DragFloat("Speed", &speed_, 0.001f);
#endif
}

void MoveForwardComponent::WriteJson(nlohmann::json& json) const {
	json["speed"] = speed_;
}

void MoveForwardComponent::ReadJson(const nlohmann::json& json) {
	if (!json.contains("speed")) {
		return;
	}
	if (!json.at("speed").is_number()) {
		return;
	}

	speed_ = json.at("speed").get<float>();
}

} // namespace KujakuEngine
```

### 3. Factory に登録する

`DirectXGame/KujakuEngine/components/BuiltinComponents.cpp` に include と登録を追加します。

```cpp
#include "MoveForwardComponent.h"

ComponentFactory::GetInstance().Register("MoveForwardComponent", []() {
	return std::make_unique<MoveForwardComponent>();
});
```

登録すると、Inspector の `Add Component` メニューに表示され、JSON Import 時にも `type: "MoveForwardComponent"` から復元できるようになります。

### 4. Visual Studio プロジェクトへ追加する

新しい `.h` / `.cpp` は、Visual Studio のプロジェクトにも追加してください。ファイルを作っただけでは `.vcxproj` に含まれず、ビルドされないことがあります。

## 新しい Scene の作り方

`Scene` を継承して、`Initialize()`、`Update()`、`Draw()`、`GetSceneName()` などを実装します。

```cpp
class MyScene : public Scene {
public:
	void Initialize() override;
	void Update() override;
	void Draw() override;
	const char* GetSceneName() const override { return "MyScene"; }
	Camera* GetEditorCamera() override;

private:
	CameraComponent* cameraComponent_ = nullptr;
};
```

最小の考え方は次の通りです。

```cpp
void MyScene::Initialize() {
	GameObject* cameraObject = CreateGameObject("Main Camera");
	cameraObject->GetTransform().translation_ = {0.0f, 0.0f, -20.0f};
	cameraComponent_ = cameraObject->AddComponent<CameraComponent>();

	GameObject* cube = CreateGameObject("Cube");
	ModelRendererComponent* renderer = cube->AddComponent<ModelRendererComponent>(&cameraComponent_->GetCamera());
	renderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Cube, "resources/monsterBall.png");

	Scene::Initialize();
}

void MyScene::Update() {
	Scene::Update();
}

void MyScene::Draw() {
	Model::PreDraw();
	Scene::Draw();
	Model::PostDraw();
}

Camera* MyScene::GetEditorCamera() {
	if (!cameraComponent_) {
		return nullptr;
	}

	return &cameraComponent_->GetCamera();
}
```

実際の `SampleScene` では、Edit 用の DebugCamera、Play 用の Main Camera、Light の適用、ModelRenderer への Camera 再設定なども行っています。新しい Scene を作る場合は、まず `SampleScene` を参考にするのが一番早いです。

作成した Scene を使うには `DirectXGame/main.cpp` の `SetCurrentScene()` を変更します。

```cpp
editorApplication->SetCurrentScene(std::make_unique<MyScene>());
```

## 主要ファイル一覧

### エントリーポイント

| ファイル | 役割 |
| --- | --- |
| `DirectXGame/main.cpp` | Windows アプリの入口。KujakuEngine 初期化、EditorApplication 呼び出し、Scene セット、終了処理を行います。 |
| `DirectXGame/KujakuEngine/KujakuEngine.h` | エンジン利用側が include する総合ヘッダです。主要クラスをまとめて include しています。 |
| `DirectXGame/KujakuEngine/KujakuEngine.cpp` | WinApp、DirectXCommon、ImGui、GraphicsPipeline、Light、Texture、Input、Time などの初期化と毎フレーム更新をまとめます。 |

### Editor

| ファイル | 役割 |
| --- | --- |
| `Editor/EditorApplication.h/.cpp` | Editor 全体の状態管理。Edit / Play、Start / Stop、Scene 所有、Scene Update の制御、起動時 JSON Import を担当します。 |
| `Editor/EditorSelection.h/.cpp` | Editor 内の選択状態を管理します。Hierarchy、Inspector、Game Window 選択が共有します。 |
| `Editor/EditorProjectPath.h/.cpp` | ProjectDir を検出し、保存・Project Window の基準パスを決めます。 |
| `Editor/SceneJsonExporter.h/.cpp` | Scene と GameObject を JSON ファイルへ分割保存します。 |
| `Editor/SceneJsonImporter.h/.cpp` | 起動時に Scene JSON と GameObject JSON を読み込み、Scene へ適用します。 |
| `Editor/ProjectWindow.h/.cpp` | Project Window の UI、ファイル一覧、右クリックメニュー、画像・モデルプレビューを担当します。 |
| `Editor/ProjectAssetClassifier.h/.cpp` | ファイル種別を分類し、Project Window 上のアイコンやプレビュー方式を決めます。 |

### ImGui / 2D

| ファイル | 役割 |
| --- | --- |
| `2d/ImGuiManager.h/.cpp` | ImGui 初期化、DockSpace、Game / Hierarchy / Inspector / Project / Console Window、Ctrl+S、Game Window 選択、ImGuizmo を担当します。 |
| `2d/Sprite.h/.cpp` | 2D Sprite 描画を担当します。 |

### Scene

| ファイル | 役割 |
| --- | --- |
| `scene/Scene.h/.cpp` | Scene 基底。GameObject 一覧の所有、Create / Add、Update / Draw / Finalize、JSON 文字列出力を持ちます。 |
| `scene/GameObject.h/.cpp` | Scene 上の Object。名前、Active、必須 Transform、Component 一覧、Lifecycle を持ちます。 |
| `scene/Component.h/.cpp` | Component 基底。Lifecycle、Inspector、JSON、Play Start / Stop、Enabled を持ちます。 |
| `scene/ComponentFactory.h/.cpp` | 型名文字列から Component を生成する Factory です。Inspector の Add Component と JSON Import で使います。 |
| `scene/RayCast.h/.cpp` | Viewport 座標から Ray を作成し、Scene 内の ModelRendererComponent へ RayCast します。Editor 選択とゲーム中判定の共通入口です。 |
| `scene/SampleScene.h/.cpp` | 現在のサンプル Scene。標準 Camera / Light GameObject、MonsterBall、Terrain、描画順、Camera 切り替えを実装しています。 |

### Components

| ファイル | 役割 |
| --- | --- |
| `components/BuiltinComponents.h/.cpp` | 標準 Component を `ComponentFactory` に登録します。 |
| `components/TransformComponent.h/.cpp` | 必須 Transform Component。WorldTransform の Inspector と JSON を担当します。 |
| `components/ModelRendererComponent.h/.cpp` | Model 描画 Component。Primitive、Texture、Model の Inspector と JSON を担当します。 |
| `components/CameraComponent.h/.cpp` | GameObject Transform を Camera に反映します。 |
| `components/DebugCameraComponent.h/.cpp` | Edit 中の DebugCamera 操作を GameObject Transform に反映します。 |
| `components/DirectionalLightComponent.h/.cpp` | DirectionalLight のデータを Inspector / JSON / GPU 反映します。 |
| `components/PointLightComponent.h/.cpp` | PointLight のデータを Inspector / JSON / GPU 反映します。 |
| `components/RotatorComponent.h/.cpp` | 回転処理のサンプル Component です。 |
| `components/TransformSnapshotComponent.h/.cpp` | Play 開始時の Transform 保存と Stop 時の復元を行います。 |

### Base

| ファイル | 役割 |
| --- | --- |
| `base/WinApp.h/.cpp` | Win32 ウィンドウ作成、メッセージ処理、リサイズ処理を担当します。 |
| `base/DirectXCommon.h/.cpp` | DX12 Device、SwapChain、Command、Fence、DescriptorHeap、BackBuffer、Game RenderTarget を管理します。 |
| `base/TextureManager.h/.cpp` | テクスチャ読み込み、SRV 作成、キャッシュ、デフォルト白テクスチャを管理します。 |
| `base/Time.h/.cpp` | フレーム時間を管理します。 |
| `base/GlobalVariables.h/.cpp` | グローバル変数ファイルの読み込みを担当します。 |
| `base/Logger.h/.cpp` | ログ出力を担当します。 |
| `base/StringUtil.h/.cpp` | 文字列変換などの補助関数です。 |

### 3D

| ファイル | 役割 |
| --- | --- |
| `3d/GraphicsPipeline.h/.cpp` | RootSignature、PSO、BlendMode、ShaderModel などの描画パイプラインを管理します。 |
| `3d/Model.h/.cpp` | 3D Model の生成、頂点バッファ、マテリアル、Texture、Draw を担当します。 |
| `3d/ModelUtil.h/.cpp` | Assimp によるモデル読み込み、Node 変換、Material 生成、Texture 解決、共通 RenderState を担当します。 |
| `3d/WorldTransform.h/.cpp` | scale / rotation / translation、World 行列、WVP、定数バッファ転送を担当します。 |
| `3d/Camera.h/.cpp` | View / Projection 行列、Camera 定数バッファを管理します。 |
| `3d/DebugCamera.h/.cpp` | Edit 用の自由操作 Camera です。 |
| `3d/DirectionalLight.h/.cpp` | DirectionalLight の GPU データを管理します。 |
| `3d/PointLight.h/.cpp` | PointLight の GPU データを管理します。 |
| `3d/SpotLight.h/.cpp` | SpotLight の GPU データを管理します。 |
| `3d/AxisIndicator.h/.cpp` | 軸表示用の補助描画です。 |
| `3d/FollowCamera.h/.cpp` | 追従 Camera 用の補助クラスです。 |
| `3d/RailCameraController.h/.cpp` | レール Camera 制御用の補助クラスです。 |

### Input / Math / Shapes / VFX / AI

| ディレクトリ | 役割 |
| --- | --- |
| `input/Input.h/.cpp` | Keyboard、Mouse、XInput Controller の入力状態を管理します。 |
| `math` | Vector、Matrix、Easing、Random、MathUtil などの数学系処理をまとめています。 |
| `shapes` | AABB、Collider、CollisionManager、Rect、ShapeUtil などの形状・当たり判定系処理をまとめています。 |
| `vfx` | InstancingModel、Particle、ParticleEmitter、ParticleField、ParticleModel などのエフェクト系処理をまとめています。 |
| `ai` | BehaviorTree、Blackboard、FSM、UtilityAI、Navigation、Steering などの AI 実験用モジュールをまとめています。現時点では Editor の中核には接続していません。 |

### Resources / Externals

| パス | 内容 |
| --- | --- |
| `DirectXGame/KujakuEngine/resources/images` | Editor アイコン、Gizmo アイコン、Project Window アイコンです。 |
| `DirectXGame/resources` | モデルやテクスチャなど、ゲーム側で使うリソースを置く想定の場所です。 |
| `DirectXGame/externals` | ImGui、ImGuizmo、DirectXTex、Assimp、nlohmann/json などの外部ライブラリです。 |

## 実装時の注意

### main.cpp に EditorMode 分岐を増やさない

Edit / Play の判断は `EditorApplication` に集約します。

避けたい形です。

```cpp
if (EditorApplication::GetInstance()->IsPlaying()) {
	scene->Update();
}
```

`main.cpp` は次の形を保ちます。

```cpp
editorApplication->BeginFrame();
editorApplication->Update();
editorApplication->Draw();
editorApplication->EndFrame();
```

### 個別 Object の Update / Draw を外側へ散らさない

Player、Enemy、Model などを `main.cpp` や `EditorApplication` から直接呼ぶのではなく、Scene の内側に置きます。

```cpp
void MyScene::Update() {
	Scene::Update();
}

void MyScene::Draw() {
	Model::PreDraw();
	Scene::Draw();
	Model::PostDraw();
}
```

### Component は自分の保存情報だけを書く

GameObject 名や Active、Component 共通情報は外側が保存します。各 Component は自分固有の値だけを `WriteJson()` / `ReadJson()` に書きます。

```cpp
void RotatorComponent::WriteJson(nlohmann::json& json) const {
	json["speed"] = speed_;
}
```

### Transform は必須 Component

Transform は GameObject が必ず持つ Component です。Inspector では普通の Component と同じ階層で表示しますが、削除はできません。

### コメントと文字コード

KujakuEngine のコメントは UTF-8 として扱います。新しくコメントを追加する場合も UTF-8 で保存してください。

### 三項演算子

このプロジェクトでは三項演算子の使用を極力控えます。条件分岐は読みやすさを優先して `if` / `else` を使います。

## よくある問題

### 起動時に ImGui の assert が出る

`ImGuiManager::Begin()`、Editor UI 描画、`ImGuiManager::End()` の対応が崩れている可能性があります。現在の基本形は `EditorApplication::BeginFrame()` から `EndFrame()` までを毎フレーム 1 回ずつ呼ぶ形です。

### OBJ 読み込み後に `face.mNumIndices == 3` の assert が出る

四角形や多角形 Face が三角形化されていない可能性があります。Assimp 読み込み時に `aiProcess_Triangulate` を使うことで、Face を三角形へ変換します。

### Assimp の未解決外部シンボルが出る

`assimp.lib` がリンクされていない、または Runtime Library 設定がプロジェクトと Assimp で合っていない可能性があります。Debug なら `/MTd`、Release なら `/MT` のように、構成ごとに揃えてください。

### 保存した Texture や Transform が再起動後に戻る

`Ctrl + S` で Scene JSON が出力されているか確認してください。Console に export ログが出ます。保存先は `DirectXGame/SceneJson/<SceneName>` です。

### Project Window にファイルが出ない

Project root の検出に失敗している可能性があります。`EditorProjectPath::DetectEditorProjectRoot()` は、`KujakuEngine.vcxproj`、`DirectXGame/KujakuEngine.vcxproj`、`KujakuEngine.sln` を手がかりに ProjectDir を探します。

### Game Window のクリック選択が効かない

現在の選択は Edit 中のみ有効です。Play 中、ImGuizmo 操作中、ModelRendererComponent がない Object、Model 頂点から AABB を作れない Object は選択対象になりません。

## 今後の拡張予定の入口

現在の構造は、次の拡張を入れやすいように分けています。

- Scene Clone
- GameObject / Component の本格的な Inspector 編集
- Hierarchy の親子関係
- Prefab
- Scene Serialize の拡張
- Undo / Redo
- Gizmo の拡張
- OBJ / glTF の Project Window からの割り当て
- Component のドラッグ & ドロップ

特に Scene Clone は、`EditorApplication` が `EditScene` と `PlayScene` を持つ形に拡張する想定です。

```text
EditScene
  ↓ Start 時に Clone
PlayScene
  ↓ Play 中だけ Update
Stop
  ↓ PlayScene 破棄
EditScene へ戻る
```

このため、今後も Editor / Play の判断は `main.cpp` ではなく `EditorApplication` に集約してください。
