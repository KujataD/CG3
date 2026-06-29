#pragma once

#include "Scene.h"
#include "../3d/Camera.h"
#include "../3d/DebugCamera.h"
#include "../3d/SpotLight.h"

namespace KujakuEngine {

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
	Camera* GetEditorCamera() override { return currentViewCamera_; }

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
	void ApplyRenderCameraToModelRenderers(const Camera* camera);

private:
	Camera camera_;
	Camera editorCamera_;
	Camera* currentViewCamera_ = &camera_;
	DebugCamera debugCamera_;
	SpotLightData spotLight_{};
};

} // namespace KujakuEngine
