#pragma once

#include "Scene.h"
#include "../3d/SpotLight.h"
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
	void Initialize() override;

	/// <summary>
	/// Play中だけ進める更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// Edit/Play共通で行う描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// Editor保存時に使うScene名を取得
	/// </summary>
	const char* GetSceneName() const override { return "SampleScene"; }

	/// <summary>
	/// Editor操作で使うCameraを取得
	/// </summary>
	Camera* GetEditorCamera() override;

	/// <summary>
	/// EditorのHierarchyからCubeを作成する
	/// </summary>
	GameObject* CreateEditorCube() override;

	/// <summary>
	/// EditorのHierarchyからSphereを作成する
	/// </summary>
	GameObject* CreateEditorSphere() override;

	/// <summary>
	/// Editor上でComponent追加後にScene固有の依存を補完する
	/// </summary>
	void OnEditorComponentAdded(GameObject* gameObject, Component* component) override;

private:
	void UpdateSceneView();
	void EnsureSceneServiceObjects();
	GameObject* FindGameObjectByName(const std::string& name);
	Camera* GetCurrentViewCamera();
	void ApplySceneLights();
	void ApplyRenderCameraToModelRenderers(const Camera* camera);

private:
	CameraComponent* gameCameraComponent_ = nullptr;
	CameraComponent* editorCameraComponent_ = nullptr;
	DebugCameraComponent* editorDebugCameraComponent_ = nullptr;
	CameraComponent* currentViewCameraComponent_ = nullptr;
	SpotLightData spotLight_{};
};

} // namespace KujakuEngine
