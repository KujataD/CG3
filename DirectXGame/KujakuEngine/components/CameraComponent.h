#pragma once

#include "../3d/Camera.h"
#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// GameObjectのTransformをCameraへ反映するComponent
/// </summary>
class CameraComponent : public Component {
public:
	const char* GetTypeName() const override { return "CameraComponent"; }
	bool AllowMultiple() const override { return false; }

	/// <summary>
	/// Cameraの定数バッファを初期化する
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// Play中のCamera行列を更新する
	/// </summary>
	void Update() override;

	/// <summary>
	/// Edit中でも最新TransformをCameraへ反映する
	/// </summary>
	void Draw() override;

	/// <summary>
	/// Inspector表示
	/// </summary>
	void DrawInspector() override;

	/// <summary>
	/// Component情報をJSON形式で書き出す
	/// </summary>
	void WriteJson(nlohmann::json& json) const override;

	/// <summary>
	/// Component情報をJSON形式で読み込む
	/// </summary>
	void ReadJson(const nlohmann::json& json) override;

	/// <summary>
	/// JSON復元後にTransformとProjectionをCameraへ反映する
	/// </summary>
	void OnAfterReadJson() override;

	/// <summary>
	/// 所有GameObjectのTransformをCameraへ反映する
	/// </summary>
	void SyncFromOwnerTransform();

	/// <summary>
	/// Cameraを取得
	/// </summary>
	Camera& GetCamera() { return camera_; }

	/// <summary>
	/// Scene表示に使えるCameraを取得
	/// </summary>
	Camera* GetSceneCamera() override { return &camera_; }

	/// <summary>
	/// Cameraを取得
	/// </summary>
	const Camera& GetCamera() const { return camera_; }

	/// <summary>
	/// Scene表示に使えるCameraを取得
	/// </summary>
	const Camera* GetSceneCamera() const override { return &camera_; }

private:
	Camera camera_;
	bool initialized_ = false;
};

} // namespace KujakuEngine
