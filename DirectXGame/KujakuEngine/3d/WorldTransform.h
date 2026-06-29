#pragma once

#include <d3d12.h>
#include <numbers>
#include <wrl.h>

#include <math/MathUtil.h>

namespace KujakuEngine {

/// <summary>
/// 定数バッファ用データ構造体（ワールド変換）
/// </summary>
struct TransformationMatrix {
	Matrix4x4 WVP;                   // ワールド・ビュー・プロジェクション合成行列
	Matrix4x4 World;                 // ワールド行列（法線変換などに使用）
	Matrix4x4 WorldInverseTranspose; // worldの逆転置行列
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
	void UpdateMatrix(const class Camera& camera, bool isBillboard = false);

	void TransferMatrix(const Camera& camera) const;

	TransformationMatrix GetMatrixData(const Camera& camera) const;
	TransformationMatrix GetBillboardMatrixData(const Camera& camera) const;

	void CalcRotationOfVelocity(const Vector3& velocity, const Vector3& deltaAngle = {0, 0, 0}, float maxRotationSpeed = 1.0f);


	/// <summary>
	/// 定数バッファの取得
	/// </summary>
	const Microsoft::WRL::ComPtr<ID3D12Resource>& GetConstBuffer() const { return transformationMatrixResource_; }

	Vector3 GetWorldPosition() const { return {matWorld_.m[3][0], matWorld_.m[3][1], matWorld_.m[3][2]}; }
	void SetWorldPosition(Vector3 worldPos) {
		if (parent_) {
			Matrix4x4 inverseParent = Inverse(parent_->matWorld_);
			Vector3 localPos = Transform(worldPos, inverseParent);

			translation_ = localPos;
		} else {
			translation_ = worldPos;
		}
	}

private:
	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
	// マッピング済みアドレス
	mutable TransformationMatrix* constMap_ = nullptr;

	// コピー禁止
	WorldTransform(const WorldTransform&) = delete;
	WorldTransform& operator=(const WorldTransform&) = delete;
};

} // namespace KujakuEngine
