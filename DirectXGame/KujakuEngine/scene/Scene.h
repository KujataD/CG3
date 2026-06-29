#pragma once

#include "GameObject.h"
#include <memory>
#include <string>
#include <vector>

namespace KujakuEngine {

class Camera;

/// <summary>
/// Scene基底クラス
/// </summary>
class Scene {
public:
	virtual ~Scene();

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize();

	/// <summary>
	/// Scene内ObjectのUpdateを実行する
	/// </summary>
	virtual void Update();

	/// <summary>
	/// Scene内ObjectのDrawを実行する
	/// </summary>
	virtual void Draw();

	/// <summary>
	/// 終了処理
	/// </summary>
	virtual void Finalize();

	/// <summary>
	/// EditからPlayへ入る直前の処理
	/// </summary>
	virtual void OnPlayStart();

	/// <summary>
	/// PlayからEditへ戻る時の処理
	/// </summary>
	virtual void OnPlayStop();

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
	GameObject* CreateGameObject(const std::string& name = "GameObject");

	/// <summary>
	/// EditorのHierarchyから空Objectを作成する
	/// </summary>
	virtual GameObject* CreateEditorEntity();

	/// <summary>
	/// EditorのHierarchyからCubeを作成する
	/// </summary>
	virtual GameObject* CreateEditorCube();

	/// <summary>
	/// EditorのHierarchyからSphereを作成する
	/// </summary>
	virtual GameObject* CreateEditorSphere();

	/// <summary>
	/// Editor上でComponent追加後にScene固有の依存を補完する
	/// </summary>
	virtual void OnEditorComponentAdded(GameObject* gameObject, Component* component);

	/// <summary>
	/// SceneへGameObjectを追加し、所有権をSceneへ移す
	/// </summary>
	GameObject* AddGameObject(std::unique_ptr<GameObject> gameObject);

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
	std::string ToJson() const;

protected:
	/// <summary>
	/// Sceneが所有するGameObject一覧
	/// 将来はHierarchy/Serialize/Scene Cloneの対象になる。
	/// </summary>
	std::vector<std::unique_ptr<GameObject>> gameObjects_;

private:
	bool initialized_ = false;
};

} // namespace KujakuEngine
