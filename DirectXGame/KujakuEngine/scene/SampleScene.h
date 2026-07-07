#pragma once

#include "Scene.h"
#include <string>

namespace KujakuEngine {

class Camera;
class CameraComponent;
class DebugCameraComponent;

/// <summary>
/// 現在のmain.cppにあったサンプル用Scene
/// </summary>
class SampleScene : public Scene {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	KUJAKU_API void Initialize() override;

	/// <summary>
	/// Play中だけ進める更新処理
	/// </summary>
	KUJAKU_API void Update() override;

	/// <summary>
	/// Edit/Play共通で行う描画処理
	/// </summary>
	KUJAKU_API void Draw() override;

	/// <summary>
	/// Editor保存時に使うScene名を取得
	/// </summary>
	const char* GetSceneName() const override { return "SampleScene"; }

	/// <summary>
	/// Editor操作で使うCameraを取得
	/// </summary>
	KUJAKU_API Camera* GetEditorCamera() override;

	/// <summary>
	/// EditorのHierarchyからCubeを作成する
	/// </summary>
	KUJAKU_API GameObject* CreateEditorCube() override;

	/// <summary>
	/// EditorのHierarchyからSphereを作成する
	/// </summary>
	KUJAKU_API GameObject* CreateEditorSphere() override;

	/// <summary>
	/// Editor上でComponent追加後にScene固有の依存を補完する
	/// </summary>
	KUJAKU_API void OnEditorComponentAdded(GameObject* gameObject, Component* component) override;

private:
	/// <summary>
	/// SceneView更新処理を行います。
	/// </summary>
	void UpdateSceneView();
	/// <summary>
	/// EnsureSceneServiceObjectsを実行します。
	/// </summary>
	void EnsureSceneServiceObjects();
	/// <summary>
	/// GameObjectByNameを検索します。
	/// </summary>
	GameObject* FindGameObjectByName(const std::string& name);
	/// <summary>
	/// CurrentViewCameraを取得します。
	/// </summary>
	Camera* GetCurrentViewCamera();
	/// <summary>
	/// ApplySceneLightsを実行します。
	/// </summary>
	void ApplySceneLights();
	/// <summary>
	/// ApplyRenderCameraToModelRenderersを実行します。
	/// </summary>
	void ApplyRenderCameraToModelRenderers(const Camera* camera);

private:
	CameraComponent* gameCameraComponent_ = nullptr;
	CameraComponent* editorCameraComponent_ = nullptr;
	DebugCameraComponent* editorDebugCameraComponent_ = nullptr;
	CameraComponent* currentViewCameraComponent_ = nullptr;
};

} // namespace KujakuEngine
