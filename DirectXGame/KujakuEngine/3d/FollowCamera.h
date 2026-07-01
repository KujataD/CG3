#pragma once
#include "Camera.h"
#include "WorldTransform.h"

namespace KujakuEngine {

/// <summary>
/// FollowCameraクラスを表します。
/// </summary>
class FollowCamera {
public:
	/// <summary>
	/// 初期化します。
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理を行います。
	/// </summary>
	void Update();

	/// <summary>
	/// Cameraを取得します。
	/// </summary>
	const Camera& GetCamera() const { return viewProjection_; }
	void SetTarget(const WorldTransform* target) { target_ = target; }

private:
	Camera viewProjection_;
	const WorldTransform* target_ = nullptr;
};

} // namespace KujakuEngine
