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
