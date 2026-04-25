#include "WorldTransform.h"
#include "../base/DirectXCommon.h"
#include "Camera.h"
#include <cassert>

namespace KujakuEngine {

void WorldTransform::Initialize() {
	// 定数バッファの生成
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferResourceDesc{};
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width = sizeof(ConstBufferDataWorldTransform);
	bufferResourceDesc.Height = 1;
	bufferResourceDesc.DepthOrArraySize = 1;
	bufferResourceDesc.MipLevels = 1;
	bufferResourceDesc.SampleDesc.Count = 1;
	bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constBuffer_));
	assert(SUCCEEDED(hr));

	// マッピング
	hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));
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

void WorldTransform::TransferMatrix(const Camera& camera) {	// WVP行列の生成
	Matrix4x4 matWVP = matWorld_ * camera.matView * camera.matProjection;

	// 定数バッファへ転送
	constMap_->WVP = matWVP;
	constMap_->World = matWorld_;
}

} // namespace KujakuEngine