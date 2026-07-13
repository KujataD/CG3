#pragma once

#include <d3d12.h>
#include <numbers>
#include <wrl.h>

#include <math/MathUtil.h>
#include "../runtime/KujakuApi.h"

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

	KUJAKU_API void UpdateMatrix(const class Camera& camera, bool isBillboard = false);

	/// <summary>
	/// ワールド行列だけを更新する
	/// </summary>
	KUJAKU_API void UpdateWorldMatrix();

	void TransferMatrix(const Camera& camera) const;
	void TransferMatrix(const Camera& camera, const Matrix4x4& worldMatrix) const;

	TransformationMatrix GetMatrixData(const Camera& camera) const;
	TransformationMatrix GetBillboardMatrixData(const Camera& camera) const;

	void CalcRotationOfVelocity(const Vector3& velocity, const Vector3& deltaAngle = {0, 0, 0}, float maxRotationSpeed = 1.0f);


	// 現在描画中のビュー(DirectXCommonのrenderViewIndex)に対応する定数バッファを返す。
	// 同一フレームで複数ビューへ描いてもWVPが上書きされないよう、ビュー毎に別バッファを持つ。
	KUJAKU_API const Microsoft::WRL::ComPtr<ID3D12Resource>& GetConstBuffer() const;

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
	// 定数バッファ(ビュー毎に別々。Scene/Gameで同じオブジェクトを描くため2本持つ)。
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_[2];
	// マッピング済みアドレス(各ビュー分)
	mutable TransformationMatrix* constMap_[2] = {nullptr, nullptr};

	// コピー禁止
	WorldTransform(const WorldTransform&) = delete;
	WorldTransform& operator=(const WorldTransform&) = delete;
};

} // namespace KujakuEngine
