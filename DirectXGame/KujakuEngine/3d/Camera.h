#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

#include "../base/WinApp.h"
#include <math/MathUtil.h>
#include "../shapes/Rect.h"

namespace KujakuEngine {

/// <summary>
/// カメラ定数バッファ用データ構造体
/// </summary>
struct ConstBufferDataCamera {
	Matrix4x4 view;       // ワールド → ビュー変換行列
	Matrix4x4 projection; // ビュー → プロジェクション変換行列
	Vector3 cameraPos;    // カメラ座標（ワールド座標）
	float pad;            // パディング
};

struct CameraForGPU {
	Vector3 worldPosition;
};

/// <summary>
/// カメラ
/// </summary>
class Camera {
public:
	// ビュー行列の設定
	Vector3 rotation_ = {0.0f, 0.0f, 0.0f};
	Vector3 translation_ = {0.0f, 0.0f, -10.0f};

	// 射影行列の設定
	float fovAngleY = 0.60f;                                                                                  // 垂直方向視野角（ラジアン）
	float aspectRatio = static_cast<float>(WinApp::kWindowWidth) / static_cast<float>(WinApp::kWindowHeight); // アスペクト比
	float nearZ = 0.1f;                                                                                       // 深度限界（手前側）
	float farZ = 1000.0f;                                                                                      // 深度限界（奥側）

	// ビュー・射影行列
	Matrix4x4 matView; // ワールドからカメラへの変換行列
	Matrix4x4 matProjection; // カメラから画面への変換行列

	Camera() = default;
	~Camera() = default;

	/// <summary>
	/// 初期化（定数バッファの生成・マッピング）
	/// </summary>
	void Initialize();

	/// <summary>
	/// 行列を更新
	/// </summary>
	void UpdateMatrix();

	/// <summary>
	/// 定数バッファの取得
	/// </summary>
	const Microsoft::WRL::ComPtr<ID3D12Resource>& GetConstBuffer() const { return constBuffer_; }

	/// <summary>
	///
	/// </summary>
	/// <returns></returns>
	const Microsoft::WRL::ComPtr<ID3D12Resource>& GetCameraForGPUResource() const { return cameraForGPUResource_; }

	/// <summary>
	/// 射影行列を更新する
	/// </summary>
	void UpdateProjectionMatrix();

	void TransferConstBuffer();

	/// <summary>
	/// カメラが映っている範囲を求める(回転固定)
	/// </summary>
	Rect GetVisibleRect(float posZ) const;

	/// <summary>
	/// カメラが映っている範囲を求める(親子関係である前提)
	/// </summary>
	Rect GetVisibleRect(float distance, float blank) const;

private:
	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
	// マッピング済みアドレス
	ConstBufferDataCamera* constMap_ = nullptr;

	// GPU用バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraForGPUResource_;
	CameraForGPU* cameraForGPUData_ = nullptr;

	// コピー禁止
	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;

	/// <summary>
	/// ビュー行列を更新する
	/// </summary>
	void UpdateViewMatrix();
};

} // namespace KujakuEngine