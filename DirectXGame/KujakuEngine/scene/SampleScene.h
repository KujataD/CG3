#pragma once

#include "Scene.h"
#include "ISceneRenderer.h"
#include <string>

namespace KujakuEngine {

class Camera;
class CameraComponent;
class DebugCameraComponent;

/// <summary>
/// 現在のmain.cppにあったサンプル用Scene
/// </summary>
class SampleScene : public Scene, public ISceneRenderer {
public:
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

	KUJAKU_API Camera* GetEditorCamera() override;

	// --- ISceneRenderer(Scene/Game 2画面描画)---
	KUJAKU_API void PrepareFrame() override;
	KUJAKU_API void RenderView(Camera* camera, bool drawEditorOverlays) override;
	KUJAKU_API Camera* GetSceneViewCamera() override;
	KUJAKU_API Camera* GetGameViewCamera() override;

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
};

} // namespace KujakuEngine
