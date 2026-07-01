#pragma once
#include "Camera.h"
#include "WorldTransform.h"

namespace KujakuEngine {

/// <summary>
/// RailCameraControllerクラスを表します。
/// </summary>
class RailCameraController {
public:
	/// <summary>
	/// 初期化します。
	/// </summary>
	void Initialize(Vector3 rotation, Vector3 position);

	/// <summary>
	/// 更新処理を行います。
	/// </summary>
	void Update();

	/// <summary>
	/// ViewMatrixを取得します。
	/// </summary>
	const Matrix4x4& GetViewMatrix() const { return camera_.matView; }
	/// <summary>
	/// WorldTransformを取得します。
	/// </summary>
	const WorldTransform* GetWorldTransform() const { return &worldTransform_; }
	void SetPosition(const Vector3& position) { worldTransform_.translation_ = position; }
	/// <summary>
	/// Rotationを設定します。
	/// </summary>
	void SetRotation(const Vector3& rotation) {
		if (abs(worldTransform_.rotation_.y - rotation.y) < std::numbers::pi_v<float>) {
			worldTransform_.rotation_ = rotation;
		}
	}

private:
	Camera camera_;

	WorldTransform worldTransform_;
};

} // namespace KujakuEngine