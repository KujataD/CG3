#include "TextureManager.h"
#include "../3d/GraphicsPipeline.h"
#include "DirectXCommon.h"
#include "WinApp.h"

namespace KujakuEngine {
void TextureManager::Initialize() { defaultWhiteTextureIndex_ = LoadTexture("resources/white1x1.png"); }
TextureManager* TextureManager::GetInstance() {
	static TextureManager instance;
	return &instance;
}

uint32_t TextureManager::LoadTexture(const std::string& filePath) { // Model::LoadTexture と同じ処理
	if (textures_.contains(filePath)) {
		return textures_[filePath].index;
	}
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();

	DirectX::ScratchImage image{};
	std::wstring filePathW(filePath.begin(), filePath.end());
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	if (FAILED(hr)) {
    std::string msg = "Failed to load texture: " + filePath + "\n";
    OutputDebugStringA(msg.c_str());
    assert(false);
}

	DirectX::ScratchImage mipImages{};
	// 1x1など最小サイズの場合はミップ生成をスキップ
	if (image.GetMetadata().width > 1 && image.GetMetadata().height > 1) {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
		assert(SUCCEEDED(hr));
	} else {
		// そのままコピー
		hr = mipImages.InitializeFromImage(*image.GetImages());
		assert(SUCCEEDED(hr));
	}

	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

	hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureResource));
	assert(SUCCEEDED(hr));

	// データ転送
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
		hr = textureResource->WriteToSubresource(UINT(mipLevel), nullptr, img->pixels, UINT(img->rowPitch), UINT(img->slicePitch));
		assert(SUCCEEDED(hr));
	}

	// SRV生成
	ID3D12DescriptorHeap* srvHeap = DirectXCommon::GetInstance()->GetSrvDescriptorHeap();
	const UINT descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	uint32_t srvIndex = srvIndexCounter_++;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += descriptorSizeSRV * srvIndex;

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += descriptorSizeSRV * srvIndex;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, cpuHandle);

	// ===== 登録 =====
	TextureData data{};
	data.resource = textureResource;
	data.cpuHandle = cpuHandle;
	data.gpuHandle = gpuHandle;
	data.index = srvIndex;

	textures_[filePath] = data;

	return srvIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandle(uint32_t index) {
	ID3D12DescriptorHeap* heap = DirectXCommon::GetInstance()->GetSrvDescriptorHeap();
	UINT size = DirectXCommon::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += size * index;
	return handle;
}

} // namespace KujakuEngine