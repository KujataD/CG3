#pragma once
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Matrix4x4.h"
#include <Windows.h>

namespace KujakuEngine{

/// <summary>
/// デバッグカメラ
/// </summary>
class DebugCamera {
public:
	
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Vector3 rotation, Vector3 translation);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	void UpdateViewMatrix();

	Matrix4x4 GetViewMatrix() { return matView_; }

private:

	static inline const float kMoveSpeed = 0.05f;
	static inline const float kRotateSpeed = 0.002f;

private:

	// ローカル回転軸
	Vector3 rotation_ = {0.0f, 0.0f, 0.0f};
	// ローカル座標
	Vector3 translation_ = {0.0f, 0.0f, -10.0f};

	// ビュー
	Matrix4x4 matView_;
	Matrix4x4 matRot_;
	
	// マウス座標
	Vector2 mousePos_;
	Vector2 prevMousePos_;
};
}
