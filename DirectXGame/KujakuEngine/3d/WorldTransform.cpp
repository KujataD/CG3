#include "WorldTransform.h"
#include "../base/DirectXCommon.h"
#include "Camera.h"
#include <cassert>
#include <numbers>

namespace KujakuEngine {

namespace {
// 現在描画中のビュー番号(範囲外は0へ丸める)。
uint32_t CurrentViewIndex() {
	uint32_t index = DirectXCommon::GetInstance()->GetRenderViewIndex();
	return index < 2 ? index : 0;
}
} // namespace

void WorldTransform::Initialize() {
	// ビュー毎に定数バッファを生成・マッピングし、単位行列で初期化する。
	matWorld_ = MakeIdentity();
	for (uint32_t viewIndex = 0; viewIndex < 2; ++viewIndex) {
		transformationMatrixResource_[viewIndex] = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));

		HRESULT hr = transformationMatrixResource_[viewIndex]->Map(0, nullptr, reinterpret_cast<void**>(&constMap_[viewIndex]));
		assert(SUCCEEDED(hr));

		constMap_[viewIndex]->WVP = MakeIdentity();
		constMap_[viewIndex]->World = MakeIdentity();
		constMap_[viewIndex]->WorldInverseTranspose = MakeIdentity();
	}
}

const Microsoft::WRL::ComPtr<ID3D12Resource>& WorldTransform::GetConstBuffer() const { return transformationMatrixResource_[CurrentViewIndex()]; }

void WorldTransform::UpdateMatrix(const Camera& camera, bool isBillboard) {
	if (isBillboard) {
		// ワールド行列の生成
		TransformationMatrix data = MakeBillboardMatrix(scale_, rotation_, translation_, camera);
		matWorld_ = data.World;
		*constMap_[CurrentViewIndex()] = data;
		return;
	}

	UpdateWorldMatrix();
	TransferMatrix(camera);
}

void WorldTransform::UpdateWorldMatrix() {
	// ローカルTransformからワールド行列を作り、親がある場合は親のワールド行列を合成する。
	matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);
	if (parent_) {
		matWorld_ = matWorld_ * parent_->matWorld_;
	}
}

void WorldTransform::TransferMatrix(const Camera& camera) const { TransferMatrix(camera, matWorld_); }

void WorldTransform::TransferMatrix(const Camera& camera, const Matrix4x4& worldMatrix) const {
	// WVP行列の生成
	Matrix4x4 matWVP = worldMatrix * camera.matView * camera.matProjection;

	// 現在ビューの定数バッファへ転送(他ビューのWVPを上書きしない)。
	TransformationMatrix* map = constMap_[CurrentViewIndex()];
	map->WVP = matWVP;
	map->World = worldMatrix;
	map->WorldInverseTranspose = Transpose(Inverse(worldMatrix));
}

TransformationMatrix WorldTransform::GetMatrixData(const Camera& camera) const {
	TransformationMatrix data;

	Matrix4x4 matWVP = matWorld_ * camera.matView * camera.matProjection;

	data.WVP = matWVP;
	data.World = matWorld_;
	data.WorldInverseTranspose = Transpose(Inverse(matWorld_));

	return data;
}

TransformationMatrix WorldTransform::GetBillboardMatrixData(const Camera& camera) const {
	return MakeBillboardMatrix(scale_, rotation_, translation_, camera);
}

void WorldTransform::CalcRotationOfVelocity(const Vector3& velocity, const Vector3& deltaAngle, float maxRotationSpeed) {
	// Y軸周り角度(θy) ...atan2(高さ, 底辺)
	float targetY = std::atan2(velocity.x, velocity.z);
	// 横軸方向の長さを求める
	float velocityXZ = Length({velocity.x, 0.0f, velocity.z});
	// X軸周り角度(θx)
	float targetX = std::atan2(-velocity.y, velocityXZ);

	if (std::abs(rotation_.x - targetX) > maxRotationSpeed) {
		rotation_.x += maxRotationSpeed;
	}

	rotation_.x = targetX;
	rotation_.y = targetY;

	rotation_ += deltaAngle;
}

} // namespace KujakuEngine
