#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "../math/Matrix4x4.h"
#include "../math/Vector3.h"

namespace KujakuEngine {

/// <summary>
/// 定数バッファ用データ構造体（ワールド変換）
/// </summary>
struct ConstBufferDataWorldTransform {
	Matrix4x4 WVP;   // ワールド・ビュー・プロジェクション合成行列
	Matrix4x4 World; // ワールド行列（法線変換などに使用）
};

/// <summary>
/// ワールド変換データ
/// </summary>
class WorldTransform {
public:
	// スケール・回転・平行移動
	Vector3 scale_ = {1.0f, 1.0f, 1.0f};
	Vector3 rotation_ = {0.0f, 0.0f, 0.0f};
	Vector3 translation_ = {0.0f, 0.0f, 0.0f};

	// ワールド行列（UpdateMatrix後に有効）
	Matrix4x4 matWorld_;

	// 親となるワールド変換へのポインタ（階層構造用）
	const WorldTransform* parent_ = nullptr;

	WorldTransform() = default;
	~WorldTransform() = default;

	/// <summary>
	/// 初期化（定数バッファの生成・マッピング）
	/// </summary>
	void Initialize();

	/// <summary>
	/// ワールド行列を更新してGPUに転送する
	/// </summary>
	/// <param name="camera">カメラ（ビュー・プロジェクション行列を取得）</param>
	void UpdateMatrix(const class Camera& camera);

	void TransferMatrix(const class Camera& camera);

	/// <summary>
	/// 定数バッファの取得
	/// </summary>
	const Microsoft::WRL::ComPtr<ID3D12Resource>& GetConstBuffer() const { return constBuffer_; }

private:
	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
	// マッピング済みアドレス
	ConstBufferDataWorldTransform* constMap_ = nullptr;

	// コピー禁止
	WorldTransform(const WorldTransform&) = delete;
	WorldTransform& operator=(const WorldTransform&) = delete;
};

} // namespace KujakuEngine