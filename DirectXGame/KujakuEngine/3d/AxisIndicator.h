#pragma once

#include "Camera.h"
#include "Model.h"
#include "WorldTransform.h"
#include <memory>
#include <string>

namespace KujakuEngine {

// 軸方向表示
/// <summary>
/// AxisIndicatorクラスを表します。
/// </summary>
class AxisIndicator {
public:
	// ビューポート矩形範囲
	static const float kViewPortTopLeftX;
	static const float kViewPortTopLeftY;
	static const float kViewPortWidth;
	static const float kViewPortHeight;
	static const float kCameraDistance;

	// モデル名
	static const std::string kModelName;

	/// <returns>シングルトンインスタンス</returns>
	/// <summary>
	/// Instanceを取得します。
	/// </summary>
	static AxisIndicator* GetInstance();

	/// <param name="targetCamera">トレースするカメラ</param>
	/// <summary>
	/// TargetCameraを設定します。
	/// </summary>
	static void SetTargetCamera(const Camera* targetCamera);

	/// <param name="isVisible">表示フラグ</param>
	/// <summary>
	/// Visibleを設定します。
	/// </summary>
	static void SetVisible(bool isVisible);

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	/// <summary>
	/// AxisIndicatorを実行します。
	/// </summary>
	AxisIndicator() = default;
	/// <summary>
	/// AxisIndicatorを実行します。
	/// </summary>
	~AxisIndicator() = default;
	/// <summary>
	/// AxisIndicatorを実行します。
	/// </summary>
	AxisIndicator(const AxisIndicator&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	AxisIndicator& operator=(const AxisIndicator&) = delete;

	// モデル
	std::unique_ptr<Model> model_;
	// 軸表示専用カメラ
	Camera camera_;
	// ワールドトランスフォーム
	WorldTransform worldTransform_;
	// トレースするカメラ
	const Camera* targetCamera_ = nullptr;
	// 表示フラグ
	bool isVisible_ = false;
	// 初期化済みフラグ
	bool isInitialized_ = false;
};

} // namespace KujakuEngine
