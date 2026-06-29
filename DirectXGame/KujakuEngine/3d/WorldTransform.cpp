#include "WorldTransform.h"
#include "../base/DirectXCommon.h"
#include "Camera.h"
#include <cassert>
#include <numbers>

namespace KujakuEngine {

void WorldTransform::Initialize() {
	// 定数バッファの生成
	transformationMatrixResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));

	// マッピング
	HRESULT hr = transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));
	assert(SUCCEEDED(hr));

	// 単位行列で初期化
	constMap_->WVP = MakeIdentity();
	constMap_->World = MakeIdentity();
	constMap_->WorldInverseTranspose = MakeIdentity();
}

void WorldTransform::UpdateMatrix(const Camera& camera, bool isBillboard) {
	if (isBillboard) {
		// ワールド行列の生成
		TransformationMatrix data = MakeBillboardMatrix(scale_, rotation_, translation_, camera);
		matWorld_ = data.World;
		*constMap_ = data;
		return;
	}

	// ワールド行列の生成
	matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

	// 親がいれば親のワールド行列を掛ける（階層構造）
	if (parent_) {
		matWorld_ = matWorld_ * parent_->matWorld_;
	}

	TransferMatrix(camera);
}

void WorldTransform::TransferMatrix(const Camera& camera) const { TransferMatrix(camera, matWorld_); }

void WorldTransform::TransferMatrix(const Camera& camera, const Matrix4x4& worldMatrix) const {
	// WVP行列の生成
	Matrix4x4 matWVP = worldMatrix * camera.matView * camera.matProjection;

	// 定数バッファへ転送
	constMap_->WVP = matWVP;
	constMap_->World = worldMatrix;
	constMap_->WorldInverseTranspose = Transpose(Inverse(worldMatrix));
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
