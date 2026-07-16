#pragma once

#include "../runtime/KujakuApi.h"
#include "GameObject.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace KujakuEngine {

class Camera;
class ColliderComponent;

class Scene {
public:
	KUJAKU_API virtual ~Scene();
	KUJAKU_API virtual void Initialize();
	KUJAKU_API virtual void Update();
	KUJAKU_API virtual void Draw();

	KUJAKU_API virtual void Finalize();

	/// <summary>
	/// EditからPlayへ入る直前の処理
	/// </summary>
	KUJAKU_API virtual void OnPlayStart();

	/// <summary>
	/// PlayからEditへ戻る時の処理
	/// </summary>
	KUJAKU_API virtual void OnPlayStop();

	/// <summary>
	/// Editor保存時に使うScene名を取得
	/// </summary>
	virtual const char* GetSceneName() const { return "Scene"; }

	virtual Camera* GetEditorCamera() { return nullptr; }

	// --- Scene/Game 2画面描画用API ---
	/// <summary>
	/// 毎フレーム1回の準備(入力反映・カメラ同期・ライト・ワールド行列など)。ビュー描画の前に呼ぶ。
	/// </summary>
	KUJAKU_API virtual void PrepareFrame();

	/// <summary>
	/// 1ビューぶんの描画。cameraでモデルを描き、drawEditorOverlaysでグリッド/アイコン/フラスタム等を足す。
	/// </summary>
	KUJAKU_API virtual void RenderView(Camera* camera, bool drawEditorOverlays);

	/// <summary>
	/// Sceneビュー(デバッグカメラ)のカメラ。既定はGetEditorCamera。
	/// </summary>
	virtual Camera* GetSceneViewCamera() { return GetEditorCamera(); }

	/// <summary>
	/// Gameビュー(メインカメラ)のカメラ。2画面対応シーンのみ非nullを返す。
	/// </summary>
	virtual Camera* GetGameViewCamera() { return nullptr; }

	/// <summary>
	/// Editor用ビルボード(アイコン)のModel/Transformを準備する。描画パス外から呼ぶこと。
	/// ランタイム生成されたComponentのアイコンも確実に表示させるため冪等に何度でも呼べる。
	/// </summary>
	KUJAKU_API void RefreshEditorBillboards();

	/// <summary>
	/// GameObjectを作成し、所有権をSceneへ持たせる
	/// </summary>
	KUJAKU_API GameObject* CreateGameObject(const std::string& name = "GameObject");

	/// <summary>
	/// EditorのHierarchyから空Objectを作成する
	/// </summary>
	KUJAKU_API virtual GameObject* CreateEditorEntity();

	/// <summary>
	/// EditorのHierarchyからCubeを作成する
	/// </summary>
	KUJAKU_API virtual GameObject* CreateEditorCube();

	/// <summary>
	/// EditorのHierarchyからSphereを作成する
	/// </summary>
	KUJAKU_API virtual GameObject* CreateEditorSphere();

	/// <summary>
	/// Editor上でComponent追加後にScene固有の依存を補完する
	/// </summary>
	KUJAKU_API virtual void OnEditorComponentAdded(GameObject* gameObject, Component* component);

	/// <summary>
	/// SceneへGameObjectを追加し、所有権をSceneへ移す
	/// </summary>
	KUJAKU_API GameObject* AddGameObject(std::unique_ptr<GameObject> gameObject);

	KUJAKU_API void RemoveGameObjectHierarchy(GameObject* gameObject);

	/// <summary>
	/// Hierarchyのドラッグ&ドロップ並び替え用。draggedをtargetの兄弟にし、直前/直後へ移動する。
	/// 必要なら親を変更(ワールド位置維持)し、ランタイム表示順(children_)と
	/// 永続化順(gameObjects_)の両方を更新する。
	/// </summary>
	KUJAKU_API bool MoveGameObjectOrder(GameObject* dragged, GameObject* target, bool insertAfter);

	KUJAKU_API GameObject* FindGameObjectByInstanceId(const std::string& instanceId) const;

	/// <summary>
	/// Scene内GameObjectのワールド行列を親子順に更新
	/// </summary>
	KUJAKU_API void UpdateWorldTransforms();

	std::vector<std::unique_ptr<GameObject>>& GetGameObjects() { return gameObjects_; }

	const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const { return gameObjects_; }

	KUJAKU_API std::string ToJson() const;

protected:
	/// <summary>
	/// Sceneが所有するGameObject一覧
	/// 将来はHierarchy/Serialize/Scene Cloneの対象になる。
	/// </summary>
	std::vector<std::unique_ptr<GameObject>> gameObjects_;

private:
	struct CollisionPairState {
		ColliderComponent* colliderA = nullptr;
		ColliderComponent* colliderB = nullptr;
		bool isTrigger = false;
	};

	void UpdateCollisions();

	bool initialized_ = false;
	std::unordered_map<std::string, CollisionPairState> collisionPairStates_;
};

} // namespace KujakuEngine
