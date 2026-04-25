#include "DirectionalLight.h"
#include "../base/DirectXCommon.h"
#include <cassert>

namespace KujakuEngine {

DirectionalLight* DirectionalLight::GetInstance() {
	static DirectionalLight instance;
	return &instance;
}

void DirectionalLight::Initialize() {
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = sizeof(DirectionalLightData);
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&lightResource_));
	assert(SUCCEEDED(hr));

	lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightMap_));

	// デフォルト値を書き込む
	*lightMap_ = DirectionalLightData{};
}

void DirectionalLight::Update() {
	// Mapしたままなので特に何もしなくて良い
	// ImGuiで GetData() を経由して値を変えれば即反映される
}

} // namespace KujakuEngine