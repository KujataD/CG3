#pragma once

#include "../math/Quaternion.h"
#include "../math/Vector3.h"
#include "../runtime/KujakuApi.h"
#include "../scene/Component.h"
#include <string>

namespace KujakuEngine {

/// <summary>
/// ターゲットを追従する三人称オービットカメラ(Cinemachineの3rd Person Follow相当)。
/// Main Camera のGameObjectへ追加して使う。Play中のみ動作する。
///
/// - 右スティックでyaw/pitchを操作(pitchはクランプ)
/// - 位置 = 注視点(ターゲット+高さオフセット)から姿勢方向へdistance離れた点
/// - 姿勢の減衰はQuaternionのSlerp、注視点の追従は指数平滑
/// - 所有GameObjectのTransformへ書き込むだけなので、既存CameraComponentがカメラへ同期する
/// </summary>
class KUJAKU_API OrbitCameraComponent : public Component {
public:
	const char* GetTypeName() const override { return "OrbitCameraComponent"; }
	bool AllowMultiple() const override { return false; }

	void Update() override;
	void OnPlayStart() override;

	void DrawInspector() override;
	void WriteJson(nlohmann::json& json) const override;
	void ReadJson(const nlohmann::json& json) override;

	void SetTargetName(const std::string& targetName) { targetName_ = targetName; }
	const std::string& GetTargetName() const { return targetName_; }

	float GetYaw() const { return yaw_; }
	float GetPitch() const { return pitch_; }

private:
	// --- 設定(保存対象) ---
	std::string targetName_;                // 追従するGameObject名
	float distance_ = 6.0f;                 // 注視点からの距離
	float pivotHeight_ = 1.5f;              // ターゲット位置へ加える注視点の高さ
	float sensitivityX_ = 2.5f;             // 右スティックX感度[rad/s]
	float sensitivityY_ = 1.8f;             // 右スティックY感度[rad/s]
	bool invertY_ = false;                  // 上下反転
	float pitchMin_ = -1.2f;                // 見上げ限界[rad]
	float pitchMax_ = 1.2f;                 // 見下ろし限界[rad]
	float positionDamping_ = 10.0f;         // 注視点追従の速さ(大=機敏, 0=無効)
	float rotationDamping_ = 18.0f;         // スティック応答の速さ(大=機敏, 0=無効)

	// --- 遮蔽回避(Cinemachine Collider相当) ---
	bool collisionEnabled_ = true;          // 障害物でカメラを引き寄せるか
	float cameraRadius_ = 0.3f;             // カメラの半径(壁からのマージン)
	float minDistance_ = 0.5f;              // 引き寄せの下限距離
	float occlusionRecovery_ = 4.0f;        // 遮蔽解除後に距離が戻る速さ
	uint32_t obstacleMask_ = 0xffffffff;    // 遮蔽物として扱うLayerのビットマスク

	// --- リセンタリング ---
	bool recenterEnabled_ = false;          // 無入力時にターゲット背後へ回り込むか
	float recenterWaitTime_ = 2.0f;         // 無入力からリセンタリング開始までの秒数
	float recenterSpeed_ = 3.0f;            // 回り込みの速さ

	// --- 実行時状態 ---
	float yaw_ = 0.0f;
	float pitch_ = 0.35f;
	Quaternion currentOrientation_ = Quaternion::Identity();
	Vector3 currentPivot_ = {0.0f, 0.0f, 0.0f};
	float currentDistance_ = 6.0f;
	float recenterTimer_ = 0.0f;
	bool snapNextUpdate_ = true;

	/// <summary>
	/// 注視点から理想位置への線分をシーンのColliderと判定し、遮蔽を考慮した距離を返します。
	/// </summary>
	float ComputeOccludedDistance(class Scene& scene, class GameObject* cameraOwner, class GameObject* target) const;
};

} // namespace KujakuEngine
