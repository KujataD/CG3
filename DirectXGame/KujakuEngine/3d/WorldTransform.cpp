#include "WorldTransform.h"
#include "../base/DirectXCommon.h"
#include "Camera.h"
#include <cassert>

namespace KujakuEngine {

void WorldTransform::Initialize() {
	// 定数バッファの生成
	transformationMatrixResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));

	// マッピング
	HRESULT hr = transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));
	assert(SUCCEEDED(hr));

	// 単位行列で初期化
	constMap_->WVP = Matrix4x4::MakeIdentity();
	constMap_->World = Matrix4x4::MakeIdentity();
}

void WorldTransform::UpdateMatrix(const Camera& camera) {
	// ワールド行列の生成
	matWorld_ = Matrix4x4::MakeAffineMatrix(scale_, rotation_, translation_);

	// 親がいれば親のワールド行列を掛ける（階層構造）
	if (parent_) {
		matWorld_ = matWorld_ * parent_->matWorld_;
	}

	TransferMatrix(camera);
}

void WorldTransform::UpdateBillboardMatrix(const Camera& camera) {
	// ワールド行列の生成
	Matrix4x4 billboardMatrix = kBackToFrontMatrix * Matrix4x4::Inverse(camera.matView);
	billboardMatrix.m[3][0] = 0.0f; // 平行移動成分は要らない
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	// 回転対応
	Matrix4x4 rotateZMatrix = Matrix4x4::MakeRotateZMatrix(rotation_.z);

	Matrix4x4 rotateMatrix = rotateZMatrix * billboardMatrix;
	Matrix4x4 scaleMatrix = Matrix4x4::MakeScaleMatrix(scale_);
	Matrix4x4 translateMatrix = Matrix4x4::MakeTranslateMatrix(translation_);

	matWorld_ = scaleMatrix * rotateMatrix * translateMatrix;

	TransferMatrix(camera);
}

void WorldTransform::TransferMatrix(const Camera& camera) { // WVP行列の生成
	Matrix4x4 matWVP = matWorld_ * camera.matView * camera.matProjection;

	// 定数バッファへ転送
	constMap_->WVP = matWVP;
	constMap_->World = matWorld_;
	constMap_->WorldInverseTranspose = Matrix4x4::Transpose(Matrix4x4::Inverse(matWorld_));
}

TransformationMatrix WorldTransform::GetMatrixData(const Camera& camera) const {
	TransformationMatrix data;

	Matrix4x4 matWVP = matWorld_ * camera.matView * camera.matProjection;

	data.WVP = matWVP;
	data.World = matWorld_;

	return data;
}

TransformationMatrix WorldTransform::GetBillboardMatrixData(const Camera& camera) const { return TransformationMatrix(); }

} // namespace KujakuEngine