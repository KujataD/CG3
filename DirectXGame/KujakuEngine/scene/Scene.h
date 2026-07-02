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

/// <summary>
/// Scene基底クラス
/// </summary>
class Scene {
public:
	/// <summary>
	/// Sceneを実行します。
	/// </summary>
	KUJAKU_API virtual ~Scene();

	/// <summary>
	/// 初期化
	/// </summary>
	KUJAKU_API virtual void Initialize();

	/// <summary>
	/// Scene内ObjectのUpdateを実行する
	/// </summary>
	KUJAKU_API virtual void Update();

	/// <summary>
	/// Scene内ObjectのDrawを実行する
	/// </summary>
	KUJAKU_API virtual void Draw();

	/// <summary>
	/// 終了処理
	/// </summary>
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

	/// <summary>
	/// Editor操作で使うCameraを取得
	/// </summary>
	virtual Camera* GetEditorCamera() { return nullptr; }

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

	/// <summary>
	/// GameObject階層をSceneから削除する
	/// </summary>
	KUJAKU_API void RemoveGameObjectHierarchy(GameObject* gameObject);

	/// <summary>
	/// instanceIdからGameObjectを検索する
	/// </summary>
	KUJAKU_API GameObject* FindGameObjectByInstanceId(const std::string& instanceId) const;

	/// <summary>
	/// PrefabファイルからGameObject階層を生成する
	/// </summary>
	KUJAKU_API GameObject* InstantiatePrefab(const std::filesystem::path& prefabPath);

	/// <summary>
	/// Scene内GameObjectのワールド行列を親子順に更新
	/// </summary>
	KUJAKU_API void UpdateWorldTransforms();

	/// <summary>
	/// GameObject一覧を取得
	/// </summary>
	std::vector<std::unique_ptr<GameObject>>& GetGameObjects() { return gameObjects_; }

	/// <summary>
	/// GameObject一覧を取得
	/// </summary>
	const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const { return gameObjects_; }

	/// <summary>
	/// Scene内のGameObject / Component情報をJSON形式で取得
	/// </summary>
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

	/// <summary>
	/// Scene内ColliderComponentの接触イベントを更新します。
	/// </summary>
	void UpdateCollisions();

	bool initialized_ = false;
	std::unordered_map<std::string, CollisionPairState> collisionPairStates_;
};

} // namespace KujakuEngine
