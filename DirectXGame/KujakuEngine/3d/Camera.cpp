#include "Camera.h"
#include "../base/DirectXCommon.h"
#include <cassert>

namespace KujakuEngine {

void Camera::Initialize() {
	// 定数バッファの生成
	// UploadHeapの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// バッファリソースの設定
	D3D12_RESOURCE_DESC bufferResourceDesc{};
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width = sizeof(ConstBufferDataCamera);
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
}

void Camera::UpdateMatrix() {
	UpdateViewMatrix();
	UpdateProjectionMatrix();
	TransferConstBuffer();
}

void Camera::UpdateViewMatrix() {
	// cameraMatrix = MakeAffineMatrix(scale=1, rotate, translate)
	// viewMatrix   = Inverse(cameraMatrix)
	Matrix4x4 cameraMatrix = Matrix4x4::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, rotation_, translation_);
	matView = Matrix4x4::Inverse(cameraMatrix);
}

void Camera::UpdateProjectionMatrix() { matProjection = Matrix4x4::MakePerspectiveFovMatrix(fovAngleY, aspectRatio, nearZ, farZ); }

void Camera::TransferConstBuffer() {
	// 定数バッファへ転送
	constMap_->view = matView;
	constMap_->projection = matProjection;
	constMap_->cameraPos = translation_;
	constMap_->pad = 0.0f;
}

Rect Camera::GetVisibleRect(float posZ) {
	float distance = posZ - translation_.z;
	float halfHeight = std::tan(fovAngleY * 0.5f) * distance;
	float halfWidth = halfHeight * aspectRatio;

	Rect r = {
	    .left = translation_.x - halfWidth,
	    .right = translation_.x + halfWidth,
	    .bottom = translation_.y - halfHeight,
	    .top = translation_.y + halfHeight,
	};

	return r;
}

} // namespace KujakuEngine