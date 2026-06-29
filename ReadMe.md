# KujakuEngine README

KujakuEngine は、DirectX 12 ベースの描画基盤に、ImGui Docking を使った簡易エディタ、Unity 風の `Scene` / `GameObject` / `Component` 構造、JSON による Scene 保存、Project Window、Transform Gizmo を追加した学習・制作向けエンジンです。

現在の設計では、`main.cpp` はできるだけ薄くし、Editor / Play の制御は `EditorApplication`、Object の更新・描画は `Scene`、個別機能は `Component` に寄せています。

## 起動フロー

```cpp
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
```

`KujakuEngine::Update()` はウィンドウメッセージ、入力、時間を更新します。`EditorApplication::Update()` は Editor UI を更新し、Play 中だけ `Scene::Update()` を呼びます。`EditorApplication::Draw()` は Edit / Play どちらでも `Scene::Draw()` を呼び、Game Window 用 RenderTarget へ描画します。

## Editor の基本操作

- `Hierarchy` の空き部分を右クリックすると `Create Entity` / `Create Cube` / `Create Sphere` を追加できます。
- `Hierarchy` の GameObject を選択すると `Inspector` に Component が表示されます。
- `Transform` は必須 Component なので削除できません。
- `Inspector` の Add Component から登録済み Component を追加できます。
- `Game` Window 左上のアイコンで Transform Gizmo の `Translate` / `Rotate` / `Scale` を切り替えます。
- `Ctrl + S` で `DirectXGame/SceneJson/<SceneName>/` に Scene / GameObject JSON を保存します。
- エディタ起動時は保存済み JSON があれば `SceneJsonImporter` が読み込み、前回保存時の GameObject / Component / Transform / ModelRenderer 設定を復元します。
- `Project` Window のファイル右クリックメニューから `Copy File Path` と `Show in Explorer` を実行できます。

## 保存構造

```text
DirectXGame/
  SceneJson/
    SampleScene/
      SampleScene.scene.json
      GameObjects/
        0000_MonsterBall.gameobject.json
```

Scene JSON は Scene 全体の構造、GameObject JSON は各 GameObject の `name` / `active` / `components` を持ちます。現在は参照解決に GUID を使っていないため、`.meta` は生成しません。

## 新しい Component の追加方法

1. `Component` を継承したクラスを `DirectXGame/KujakuEngine/components/` に追加します。
2. `GetTypeName()`、必要なら `Initialize()` / `Update()` / `Draw()` / `DrawInspector()` / `WriteJson()` / `ReadJson()` を実装します。
3. Editor から追加したい場合は `BuiltinComponents.cpp` の `RegisterBuiltinComponents()` に登録します。
4. `SceneJsonImporter.cpp` や `ImGuiManager.cpp` の Inspector 本体には、Component 個別処理を追加しません。

Component の JSON は共通で `type` / `enabled` / `properties` を持ちます。`type` と `enabled` は基底 `Component` が扱い、各 Component は `properties` の中身だけを `WriteJson()` / `ReadJson()` で扱います。

```json
{
  "type": "RotatorComponent",
  "enabled": true,
  "properties": {
    "speed": 0.02
  }
}
```

```cpp
class BlinkComponent : public Component {
public:
    const char* GetTypeName() const override { return "BlinkComponent"; }
    void Update() override;
    void DrawInspector() override;
    void WriteJson(nlohmann::json& json) const override;
    void ReadJson(const nlohmann::json& json) override;
};
```

## 新しい Scene の追加方法

`Scene` を継承し、`Initialize()` で初期 GameObject を作成、`Update()` で `Scene::Update()`、`Draw()` で描画前後処理と `Scene::Draw()` を呼びます。Editor の Camera が必要な場合は `GetEditorCamera()` を返します。

```cpp
class MyScene : public Scene {
public:
    void Initialize() override {
        camera_.Initialize();
        GameObject* object = CreateGameObject("Cube");
        object->AddComponent<ModelRendererComponent>(&camera_)
            ->SetPrimitive(ModelRendererComponent::PrimitiveType::Cube, "resources/monsterBall.png");
        Scene::Initialize();
    }

    void Update() override {
        Scene::Update();
    }

    void Draw() override {
        Model::PreDraw();
        Scene::Draw();
        Model::PostDraw();
    }

    const char* GetSceneName() const override { return "MyScene"; }
    Camera* GetEditorCamera() override { return &camera_; }

private:
    Camera camera_;
};
```

## ファイル別リファレンス

`.h` は公開インターフェース、`.cpp` は実装です。ペアになっているものは同じ行にまとめています。

### 入口・統合

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `KujakuEngine.sln` | Visual Studio ソリューション。 | `DirectXGame/KujakuEngine.vcxproj` をビルド対象にします。 | Visual Studio で開いて `Debug x64` を実行します。 |
| `DirectXGame/KujakuEngine.vcxproj` | エンジン本体のプロジェクト設定。 | include path、lib、ImGui、ImGuizmo、Assimp などを登録します。 | 新規 `.cpp` を追加したらプロジェクトにも追加します。 |
| `DirectXGame/main.cpp` | Windows アプリのエントリーポイント。 | Engine 初期化、`EditorApplication` 初期化、Scene 設定、メインループだけを持ちます。 | `editorApplication->SetCurrentScene(std::make_unique<SampleScene>());` |
| `DirectXGame/KujakuEngine/KujakuEngine.h` / `.cpp` | エンジン初期化・終了・フレーム更新の統合入口。 | Window、DirectX、ImGui、Pipeline、Light、Texture、Input、Random、GlobalVariables、Time を順に初期化します。 | `KujakuEngine::Initialize();` / `KujakuEngine::Update();` |

### Editor

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `Editor/EditorApplication.h` / `.cpp` | Editor 全体の状態管理。 | `EditorMode::Edit` / `Play`、Start / Stop、Scene 所有、Play 中だけ Update を流す判断を持ちます。 | `EditorApplication::GetInstance()->Start();` |
| `Editor/EditorSelection.h` / `.cpp` | Editor の選択状態。 | 選択中 `GameObject*` を保持します。所有権は `Scene` が持ちます。 | `EditorSelection::GetInstance()->SetSelectedGameObject(object);` |
| `Editor/EditorProjectPath.h` / `.cpp` | ProjectDir 検出とパス正規化。 | 実行時カレントから `DirectXGame` 相当のプロジェクトルートを探します。 | `DetectEditorProjectRoot();` |
| `Editor/SceneJsonExporter.h` / `.cpp` | Scene / GameObject JSON の保存。 | `Scene::ToJson()` と Component の `WriteJson()` を使い、Scene ごと・GameObject ごとにファイルを出します。 | `SceneJsonExporter::ExportScene(*scene, root);` |
| `Editor/SceneJsonImporter.h` / `.cpp` | Scene JSON の読み込み。 | Scene 起動後に JSON を読み、GameObject / Component / Transform / ModelRenderer などを復元します。 | `SceneJsonImporter::ImportScene(*scene, root);` |
| `Editor/ProjectWindow.h` / `.cpp` | Project Window。 | ProjectDir 以下を列挙し、画像・モデル・音声・フォルダを分類して表示します。右クリックメニューも持ちます。 | `projectWindow_.Draw();` |
| `Editor/ProjectAssetClassifier.h` / `.cpp` | Project Window 用のファイル分類。 | 拡張子とフォルダ状態からアイコン、画像プレビュー、モデルプレビューを決めます。 | `classifier.Classify(path);` |

### Scene / GameObject / Component

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `scene/Scene.h` / `.cpp` | Scene 基底クラス。 | `std::vector<std::unique_ptr<GameObject>>` を所有し、`Initialize` / `Update` / `Draw` / `Finalize` を GameObject に流します。 | `GameObject* cube = scene->CreateEditorCube();` |
| `scene/SampleScene.h` / `.cpp` | 現在のサンプル Scene。 | Camera、DebugCamera、Light を初期化し、MonsterBall と Terrain を GameObject として作ります。 | `editorApplication->SetCurrentScene(std::make_unique<SampleScene>());` |
| `scene/GameObject.h` / `.cpp` | Scene 上の Object 単位。 | 必須 `TransformComponent` と複数 Component を持ち、Component の Update / Draw を順に呼びます。 | `object->AddComponent<RotatorComponent>();` |
| `scene/Component.h` / `.cpp` | Component 基底クラス。 | `Initialize` / `Update` / `Draw` / `DrawInspector` / `WriteJson` / `ReadJson` / `OnAfterReadJson` を virtual にして、Editor・実行処理・保存復元の共通入口にします。 | `class MyComponent : public Component { ... };` |
| `scene/ComponentFactory.h` / `.cpp` | Component の文字列生成。 | `typeName` と生成関数を登録し、Editor の Add Component や Import で生成します。 | `ComponentFactory::GetInstance().Create("RotatorComponent");` |
| `components/BuiltinComponents.h` / `.cpp` | 組み込み Component の登録。 | `TransformSnapshotComponent`、`RotatorComponent`、`ModelRendererComponent` などを `ComponentFactory` へ登録します。 | `RegisterBuiltinComponents();` |
| `components/TransformComponent.h` / `.cpp` | 必須 Transform Component。 | 内部に `WorldTransform` を持ち、Inspector で translate / rotate / scale を編集し、JSON に保存します。 | `object->GetTransformComponent()->GetTransform().translation_ = {0, 1, 0};` |
| `components/ModelRendererComponent.h` / `.cpp` | Model 描画 Component。 | `Model` と `Camera` を持ち、GameObject の Transform で描画します。Primitive は Cube / Sphere / None / Custom を選べます。 | `renderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Cube, "resources/monsterBall.png");` |
| `components/RotatorComponent.h` / `.cpp` | 回転テスト用 Component。 | Play 中の `Update()` で owner の Transform を回転させ、速度を Inspector / JSON に対応させます。 | `object->AddComponent<RotatorComponent>()->SetSpeed(0.05f);` |
| `components/TransformSnapshotComponent.h` / `.cpp` | Play 前後の Transform 復元。 | `OnPlayStart()` で Edit 状態を保存し、`OnPlayStop()` で戻します。 | Play 停止時に Transform を Edit 時の状態へ戻したい Object に追加します。 |

### ImGui / 2D

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `2d/ImGuiManager.h` / `.cpp` | ImGui 初期化と Editor UI 描画。 | DockSpace、Game、Hierarchy、Inspector、Console、Project Window、Ctrl+S、ImGuizmo を描画します。Start / Stop は `EditorApplication` を呼ぶだけにしています。 | `ImGuiManager::GetInstance()->DrawEditor();` |
| `2d/Sprite.h` / `.cpp` | 2D Sprite 描画。 | 頂点・Index・Material・Transform 用 Buffer を作り、TextureManager の SRV index で描画します。 | `Sprite* sprite = Sprite::Create(textureIndex, {0, 0});` |

### 3D / Rendering

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `3d/GraphicsPipeline.h` / `.cpp` | RootSignature / PSO / Shader 管理。 | DXC で HLSL をコンパイルし、Object / Particle / Instancing / Wireframe の PSO を BlendMode ごとに作ります。 | `GraphicsPipeline::GetInstance()->SetCommandList(PipelineType::kObject3d, BlendMode::kNormal);` |
| `3d/Model.h` / `.cpp` | 通常 3D Model。 | Assimp 経由で OBJ / glTF を読み込み、または Cube / Sphere / Plane などの頂点を生成して描画します。Root Node の local matrix も描画に適用します。 | `std::unique_ptr<Model> model(Model::CreateFromGlTF("plane"));` |
| `3d/ModelUtil.h` / `.cpp` | Model 読み込み共通処理。 | Assimp の Scene / Mesh / Material / Node をエンジン用 `ModelData` に変換し、Texture index を解決します。 | `ModelUtil::TryLoadModelFile("Resources", "plane.gltf", data);` |
| `3d/WorldTransform.h` / `.cpp` | World 行列と Transform 定数バッファ。 | scale / rotation / translation から `matWorld_` を作り、Camera と合成した WVP を GPU に転送します。 | `worldTransform.UpdateMatrix(camera);` |
| `3d/Camera.h` / `.cpp` | View / Projection 管理。 | Camera の Transform と投影パラメータから行列を作り、GPU 用 Buffer に転送します。 | `camera.translation_ = {0, 0, -20}; camera.UpdateMatrix();` |
| `3d/DebugCamera.h` / `.cpp` | マウス操作用 Debug Camera。 | 入力から回転・移動を更新し、`SampleScene` の Camera へ反映します。 | `debugCamera_.Update();` |
| `3d/FollowCamera.h` / `.cpp` | 追従 Camera。 | 対象 `WorldTransform` を参照し、Camera を追従させる用途の薄い Controller です。 | `followCamera.SetTarget(&playerTransform);` |
| `3d/RailCameraController.h` / `.cpp` | レール移動用 Camera Controller。 | 内部 `WorldTransform` の位置・回転から Camera View を更新します。 | `railCamera.SetPosition({0, 5, -10});` |
| `3d/DirectionalLight.h` / `.cpp` | 平行光源。 | Singleton で Light Buffer を持ち、色・方向・強度を GPU に転送します。 | `DirectionalLight::GetInstance()->GetData().intensity = 0.5f;` |
| `3d/PointLight.h` / `.cpp` | 点光源。 | 最大 `kMaxPointLight` 個の Light を配列で GPU に渡します。 | `PointLight::GetInstance()->AddLight(light);` |
| `3d/SpotLight.h` / `.cpp` | スポットライト。 | `SpotLightData` を Buffer に保持し、位置・方向・角度・減衰を渡します。 | `SpotLight::GetInstance()->SetLight(&spotLight);` |
| `3d/AxisIndicator.h` / `.cpp` | 軸方向表示。 | 対象 Camera を参照し、小さな軸モデルを専用 Camera / Viewport で描画します。 | `AxisIndicator::SetTargetCamera(&camera);` |

### Base

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `base/WinApp.h` / `.cpp` | Win32 Window 管理。 | Window Class 登録、Window 作成、Message 処理、Window 破棄を担当します。 | `WinApp::GetInstance()->CreateGameWindow(title);` |
| `base/DirectXCommon.h` / `.cpp` | DirectX 12 共通基盤。 | Device、Command、SwapChain、DescriptorHeap、Depth、Game Window 用 RenderTarget、Fence を管理します。 | `DirectXCommon::GetInstance()->BeginGameRender();` |
| `base/TextureManager.h` / `.cpp` | Texture 読み込みと SRV 管理。 | DirectXTex で画像を読み、共通 SRV Heap に登録し、path ごとの cache を持ちます。 | `uint32_t tex = TextureManager::GetInstance()->LoadTexture("resources/monsterBall.png");` |
| `base/Time.h` / `.cpp` | DeltaTime 管理。 | `QueryPerformanceCounter` でフレーム間時間を計測します。 | `float dt = Time::GetDeltaTime();` |
| `base/GlobalVariables.h` / `.cpp` | JSON ベースのグローバル変数。 | group / key / value を `Resources/GlobalVariables/` に保存・読み込みします。 | `GlobalVariables::GetInstance()->AddItem("Player", "Speed", 1.0f);` |
| `base/Logger.h` / `.cpp` | ログ出力。 | 初期化したログファイルへ文字列を出力します。 | `Logger::Log("loaded texture");` |
| `base/StringUtil.h` / `.cpp` | 文字列変換。 | `std::string` と `std::wstring` を変換します。 | `StringUtil::ToWString("Kujaku");` |

### Input

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `input/Input.h` / `.cpp` | キーボード・マウス・XInput 管理。 | DirectInput と XInput の現在状態・前フレーム状態を保持し、Trigger / Release を判定します。 | `if (Input::GetKeyTrigger(DIK_SPACE)) { ... }` |

### Math

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `math/Vector2.h` / `.cpp` | 2D Vector。 | 四則演算、Normalize、Dot、行列変換を提供します。 | `Vector2 p = Vector2::Normalize(v);` |
| `math/Vector3.h` / `.cpp` | 3D Vector。 | 3D 座標・方向の基本型です。演算子は一部 `MathUtil` 側にもあります。 | `Vector3 pos{0, 1, 0};` |
| `math/Vector4.h` | 4D Vector / Color。 | `x,y,z,w` の単純な構造体です。 | `Vector4 color{1, 1, 1, 1};` |
| `math/Matrix3x3.h` / `.cpp` | 2D 用 3x3 行列。 | 2D の scale / rotate / translate / affine / viewport を作ります。 | `Matrix3x3::MakeAffineMatrix(scale, rotate, translate);` |
| `math/Matrix4x4.h` / `.cpp` | 3D 用 4x4 行列。 | 行列の四則演算と 3D 変換の土台です。 | `Matrix4x4 world = MakeAffineMatrix(scale, rotate, translate);` |
| `math/MathUtil.h` / `.cpp` | 数学ユーティリティ。 | Dot、Cross、Normalize、Lerp、Bezier、Project、Inverse、Perspective、Billboard などを提供します。 | `Vector3 dir = Normalize(target - pos);` |
| `math/Random.h` / `.cpp` | 乱数。 | `std::mt19937_64` を使い、整数・浮動小数の範囲乱数を返します。 | `float r = Random::GetRandom(0.0f, 1.0f);` |
| `math/Easing.h` / `.cpp` | Easing。 | `EaseType` に応じて補間係数を変換します。 | `EaseUtil::EaseLerp(0.0f, 10.0f, t, EaseUtil::EaseType::OutQuad);` |

### Shape / Collision

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `shapes/AABB.h` / `.cpp` | AABB / OBB データ。 | AABB の min/max 補正、OBB の回転軸更新と World 行列生成を持ちます。 | `aabb.SwapMinMax();` |
| `shapes/Rect.h` / `.cpp` | 矩形データ。 | `left` / `right` / `bottom` / `top` の単純な範囲です。 | `Rect visible = camera.GetVisibleRect(0.0f);` |
| `shapes/ShapeUtil.h` / `.cpp` | 図形と衝突判定。 | Sphere、Plane、Segment、Ray、Line などの判定と CatmullRom 補間を提供します。 | `ShapeUtil::IsCollision(sphereA, sphereB);` |
| `shapes/Collider.h` / `.cpp` | 衝突対象の基底クラス。 | 半径、属性、マスク、`OnCollision()`、World 位置取得を virtual にします。 | Player などが継承して `GetWorldPosition()` を返します。 |
| `shapes/CollisionManager.h` / `.cpp` | Collider 同士の判定管理。 | 登録された Collider の組み合わせを走査し、属性・マスクに合う相手を判定します。 | `collisionManager.AddCollider(player); collisionManager.Update();` |

### VFX / Instancing

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `vfx/Particle.h` | 1 Particle のデータ。 | 位置・速度・寿命・色を持ち、`Update()` で移動とフェードを行います。 | `particle.Update(Time::GetDeltaTime());` |
| `vfx/ParticleField.h` / `.cpp` | Particle に影響する加速度 Field。 | AABB 範囲と加速度を持ちます。 | `emitter.AddField(field);` |
| `vfx/ParticleEmitter.h` / `.cpp` | Particle 生成・更新。 | Box、Model Edge、Segment Edge から Particle を生成し、`ParticleModel` へ instance を積みます。 | `emitter.Update(dt, camera); emitter.Draw();` |
| `vfx/ParticleModel.h` / `.cpp` | Particle 用 Instancing Model。 | 最大 10000 個の instance buffer を持ち、Particle 描画用 PSO で描画します。 | `model->AddInstanceParticle(matrix, color);` |
| `vfx/InstancingModel.h` / `.cpp` | 汎用 Instancing Model。 | 最大 1000 個の instance をまとめて描画します。Root Node local matrix を instance 行列に適用します。 | `instancing->AddInstanceModel(matrix, color);` |

### AI

AI 系は現在 Editor / GameObject へ完全統合する前のライブラリ層です。GameObject / Component から呼ぶ場合は、今後 `AIControllerComponent` のような Component を作って接続すると自然です。

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `ai/AI.h` | AI 関連 include の入口。 | AI 機能をまとめて include するためのヘッダです。 | `#include <KujakuEngine.h>` 経由で利用します。 |
| `ai/navigation/Grid.h` / `.cpp` | 経路探索用 Grid。 | `GridCell` の `walkable` と `additionalCost` を 2D 配列で持ちます。 | `grid.ResizeGridCells(20, 20);` |
| `ai/navigation/SearchNode.h` | 探索ノード。 | Grid index、cost、parent など、AStar / ThetaStar の内部で使うデータです。 | 経路探索内部で利用します。 |
| `ai/navigation/NavigationUtil.h` / `.cpp` | ナビゲーション補助。 | Manhattan / Euclidean の heuristic を計算します。 | `NavigationUtil::Heuristic(a, b);` |
| `ai/navigation/pathfinding/AStar.h` / `.cpp` | A* 経路探索。 | Grid を参照し、start から goal までの GridIndex 配列を返します。 | `path = aStar.FindPath(start, goal);` |
| `ai/navigation/pathfinding/ThetaStar.h` / `.cpp` | Theta* 経路探索。 | 見通し判定を使い、A* より滑らかな経路を返します。Agent radius も設定できます。 | `thetaStar.SetAgentRadius(0.5f);` |
| `ai/navigation/steer/ISteeringBehavior.h` / `.cpp` | Steering 基底。 | `SteeringContext` を受け取り、操舵力 `Vector3` を返す interface です。 | `behavior.Calculate(context);` |
| `ai/navigation/steer/SeekBehavior.h` / `.cpp` | 目標へ向かう Steering。 | target 方向へ速度を寄せる力を返します。 | 追跡 AI の Component 内で使います。 |
| `ai/navigation/steer/ArriveBehavior.h` / `.cpp` | 目標へ減速しながら近づく Steering。 | 到着距離に応じて速度を落とします。 | 目的地で止まりたい移動に使います。 |
| `ai/navigation/steer/AvoidanceBehavior.h` / `.cpp` | 障害回避 Steering。 | 障害物を避ける方向の力を返します。 | 障害物リストを持つ AI で使います。 |
| `ai/navigation/steer/CohesionBehavior.h` / `.cpp` | 群れの中心へ向かう Steering。 | 周囲の仲間の中心へ寄せます。 | 群れ移動に使います。 |
| `ai/navigation/steer/AligmentBehavior.h` / `.cpp` | 群れの向きを合わせる Steering。 | 周囲の平均速度へ合わせます。 | 群れ移動に使います。 |
| `ai/navigation/steer/SeparationBehavior.h` / `.cpp` | 近すぎる相手から離れる Steering。 | 周囲との距離を取り、重なりを防ぎます。 | 群れ移動に使います。 |
| `ai/navigation/steer/WanderBehavior.h` / `.cpp` | ランダム散策 Steering。 | ランダムな方向へ自然に揺れる移動力を返します。 | 徘徊 AI に使います。 |
| `ai/charactor/Core/AIContext.h` | AI 実行時 Context。 | Blackboard など、AI ノードに渡す共有情報をまとめます。 | `AIContext context{...};` |
| `ai/charactor/Core/Blackboard.h` / `.cpp` | AI 用 Key-Value Store。 | `int` / `float` / `bool` / `string` / Vec / Matrix を variant で持ちます。 | `blackboard.SetValue("hp", 10);` |
| `ai/charactor/behaviorTree/BTStatus.h` | BehaviorTree の戻り値。 | Success / Failure / Running などの状態を表します。 | `return BTStatus::Success;` |
| `ai/charactor/behaviorTree/BTNode.h` / `.cpp` | BehaviorTree Node 基底。 | `Tick()` が `OnTick()` を呼び、`Reset()` を持ちます。 | `class AttackNode : public BTNode { ... };` |
| `ai/charactor/behaviorTree/BehaviorTree.h` / `.cpp` | BehaviorTree 本体。 | root node を所有し、`Tick(context)` で木を実行します。 | `tree.SetRoot(std::move(root)); tree.Tick(context);` |
| `ai/charactor/behaviorTree/Composite/CompositeNode.h` / `.cpp` | 複数 child を持つ BT Node。 | `std::vector<std::unique_ptr<BTNode>>` と current index を持ちます。 | `selector->AddChild(std::make_unique<ActionNode>(...));` |
| `ai/charactor/behaviorTree/Composite/SelectorNode.h` / `.cpp` | Selector Node。 | child を順に実行し、成功したものを採用します。 | 条件分岐に使います。 |
| `ai/charactor/behaviorTree/Composite/SequenceNode.h` / `.cpp` | Sequence Node。 | child を順に実行し、途中失敗で失敗します。 | 一連の行動に使います。 |
| `ai/charactor/behaviorTree/Decorator/Decorator.h` / `.cpp` | Decorator Node 基底。 | child 1つを包み、条件反転や制限などを追加する拡張口です。 | `decorator->SetChild(std::move(node));` |
| `ai/charactor/behaviorTree/Leaf/LeafNode.h` / `.cpp` | Leaf Node 基底。 | child を持たない行動・条件用の基底です。 | Action / Condition の親にします。 |
| `ai/charactor/behaviorTree/Leaf/ActionNode.h` / `.cpp` | 関数で行動を書く Leaf Node。 | `std::function<BTStatus(AIContext&)>` を保持します。 | `ActionNode node([](AIContext&) { return BTStatus::Success; });` |
| `ai/charactor/fsm/IState.h` | FSM State interface。 | `Enter` / `Update` / `Exit` を owner 型ごとに実装します。 | `class IdleState : public IState<Player> { ... };` |
| `ai/charactor/fsm/StateMachine.h` | 汎用 StateMachine。 | state id と state object を map で持ち、変更時に Exit / Enter を呼びます。 | `stateMachine.ChangeState(PlayerState::Idle, player);` |
| `ai/charactor/utility/UtilityAI.h` / `.cpp` | Utility AI。 | option ごとの score 関数を評価し、最大 score の option を選びます。 | `utilityAI.SelectBest(context);` |

### Shader

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `Resources/shader/Object3d.VS.hlsl` / `Object3d.PS.hlsl` / `Object3d.hlsli` | 3D Object 用 Shader。 | Vertex / Material / Light / Camera 情報を受けて通常モデルを描画します。 | `GraphicsPipeline` の Object3d PSO から使われます。 |
| `Resources/shader/Particle.VS.hlsl` / `Particle.PS.hlsl` / `Particle.hlsli` | Particle 用 Shader。 | Instance Buffer の WVP / World / color を使って大量描画します。 | `ParticleModel::PreDraw()` から使われます。 |
| `Resources/shaders/ObjVS.hlsl` / `ObjPS.hlsl` / `Obj.hlsli` | OBJ 系描画 Shader。 | OBJ モデルの頂点・UV・法線・Material を扱います。 | 旧または追加の OBJ pipeline 用です。 |
| `Resources/shaders/SpriteVS.hlsl` / `SpritePS.hlsl` / `SpriteSRGBOutputPS.hlsl` / `Sprite.hlsli` | Sprite 用 Shader。 | 2D 頂点と UV transform、Texture を合成します。 | `Sprite::Draw()` で使います。 |
| `Resources/shaders/PrimitiveVS.hlsl` / `PrimitivePS.hlsl` / `Primitive.hlsli` | Primitive 用 Shader。 | 基本形状描画向けの最小 Shader 群です。 | Cube / Sphere などの簡易描画拡張に使います。 |
| `Resources/shaders/ShapeVS.hlsl` / `ShapePS.hlsl` / `ShapeSRGBOutputPS.hlsl` / `Shape.hlsli` | Shape 用 Shader。 | デバッグ形状や 2D/3D shape 表示向けです。 | Collision 可視化などに使えます。 |
| `Resources/shaders/TerrainVS.hlsl` / `TerrainPS.hlsl` / `Terrain.hlsli` | Terrain 用 Shader。 | 地形描画向けの Shader 群です。 | Terrain 専用 Pipeline を作る場合に使います。 |

### Engine Resource

| ファイル | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `KujakuEngine/resources/images/icon_guizmo_translate.png` | Translate Gizmo ボタン画像。 | `TextureManager` で読み込み、`ImGui::ImageButton` に渡します。 | Game Window の移動アイコン。 |
| `KujakuEngine/resources/images/icon_guizmo_rotate.png` | Rotate Gizmo ボタン画像。 | `TextureManager` で読み込み、`ImGui::ImageButton` に渡します。 | Game Window の回転アイコン。 |
| `KujakuEngine/resources/images/icon_guizmo_scale.png` | Scale Gizmo ボタン画像。 | `TextureManager` で読み込み、`ImGui::ImageButton` に渡します。 | Game Window の拡縮アイコン。 |
| `KujakuEngine/resources/images/icon_folder_fill.png` | Project Window の中身ありフォルダアイコン。 | `ProjectAssetClassifier` がフォルダ状態に応じて選びます。 | Project Window 表示。 |
| `KujakuEngine/resources/images/icon_folder_empty.png` | Project Window の空フォルダアイコン。 | `ProjectAssetClassifier` がフォルダ状態に応じて選びます。 | Project Window 表示。 |
| `KujakuEngine/resources/images/icon_file.png` | Project Window の通常ファイルアイコン。 | 未分類ファイルに使います。 | Project Window 表示。 |
| `KujakuEngine/resources/images/icon_file_audio.png` | Project Window の音声ファイルアイコン。 | `.wav` などの音声拡張子に使います。 | Project Window 表示。 |

### External Libraries

| ディレクトリ | 役割 | 簡易的な実装方法 | 使用例 |
| --- | --- | --- | --- |
| `externals/imgui` | 現在ビルドに使う ImGui 本体と Win32 / DX12 backend。 | `ImGuiManager` が context、backend、DockSpace、Editor UI を管理します。 | `ImGui::Begin("Inspector");` |
| `externals/imgui-1.92.8-docking` | Docking 版 ImGui のソース一式。 | 参照・移行用のコピーです。 | API 差分確認に使います。 |
| `externals/ImGuizmo-1.9` | Transform Gizmo。 | `ImGuiManager::DrawTransformGizmo()` から `ImGuizmo::Manipulate()` を呼びます。 | 選択 GameObject の Transform 編集。 |
| `externals/DirectXTex` | Texture 読み込み。 | `TextureManager` が画像ロードと mip upload に使います。 | PNG / JPG / DDS などの読み込み。 |
| `externals/assimp` | Model 読み込み。 | `ModelUtil` が OBJ / glTF の Mesh / Material / Node を読むために使います。 | `Model::CreateFromGlTF("plane");` |
| `externals/nlohmann` | JSON。 | `GlobalVariables`、Scene JSON import/export で使います。 | `nlohmann::json j;` |

## よく使うコード例

### GameObject を作成して描画 Component を付ける

```cpp
GameObject* cube = scene->CreateGameObject("Cube");
cube->GetTransform().translation_ = {0.0f, 0.0f, 0.0f};

ModelRendererComponent* renderer = cube->AddComponent<ModelRendererComponent>(&camera);
renderer->SetPrimitive(ModelRendererComponent::PrimitiveType::Cube, "resources/monsterBall.png");
```

### Play 中だけ回転させる

```cpp
GameObject* ball = scene->CreateGameObject("MonsterBall");
ball->AddComponent<TransformSnapshotComponent>();
RotatorComponent* rotator = ball->AddComponent<RotatorComponent>();
rotator->SetSpeed(0.02f);
```

### 直接 Model を描画する

```cpp
std::unique_ptr<Model> model(Model::CreateFromGlTF("plane", ShaderModel::kBlingPhongReflection));
WorldTransform transform;
transform.Initialize();

Model::PreDraw();
model->Draw(transform, camera);
Model::PostDraw();
```

### Texture を読み込んで Sprite を作る

```cpp
uint32_t textureIndex = TextureManager::GetInstance()->LoadTexture("resources/monsterBall.png");
Sprite* sprite = Sprite::Create(textureIndex, {100.0f, 100.0f}, 128.0f, 128.0f);

Sprite::PreDraw();
sprite->Draw();
Sprite::PostDraw();
```

### A* で経路を取る

```cpp
Grid grid;
grid.ResizeGridCells(20, 20);
grid.cells[5][5].walkable = false;

AStar aStar;
aStar.Init(&grid);
std::vector<GridIndex> path = aStar.FindPath({0, 0}, {10, 10});
```

### BehaviorTree の最小構成

```cpp
Blackboard blackboard;
AIContext context{blackboard};

auto root = std::make_unique<SelectorNode>();
root->AddChild(std::make_unique<ActionNode>([](AIContext&) {
    return BTStatus::Success;
}));

BehaviorTree tree;
tree.SetRoot(std::move(root));
tree.Tick(context);
```

## 現在の設計メモ

- `main.cpp` に EditorMode 判定を増やさず、Play / Edit の判断は `EditorApplication` に集約しています。
- `Scene` が GameObject 一覧を所有し、Update / Draw の入口になっています。
- `GameObject` は `TransformComponent` を必ず持ちます。
- `SceneJsonImporter` は Component の中身を直接読まず、`ComponentFactory` で生成して `ReadJson()` を呼びます。
- `SceneJsonExporter` は Component の共通 JSON ラッパーを使い、固有値は各 Component の `WriteJson()` に任せます。
- `ModelRendererComponent` は現時点では Cube / Sphere / None / Custom に対応しています。OBJ / glTF を Editor 上から選ぶ仕組みは今後の拡張予定です。
- Stop 時の完全な Scene Clone 復元は未実装です。現在は `TransformSnapshotComponent` が Transform の復元を担当します。
- AI はまだ Editor / Component と完全統合していません。次に進めるなら `AIControllerComponent` のような Component を作り、Blackboard / BehaviorTree / UtilityAI を GameObject に接続するのが自然です。
