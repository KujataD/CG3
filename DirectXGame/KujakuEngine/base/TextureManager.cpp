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
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
	textureResource.Attach(CreateTextureResource(device, metadata));

	// データ転送
	auto intermediate = UploadTextureData(textureResource.Get(), mipImages, DirectXCommon::GetInstance()->GetCommandList());

	DirectXCommon::GetInstance()->ExecuteCommandAndWait();

	if (intermediate) {
		intermediate->Release();
		intermediate = nullptr;
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

	// 登録
	TextureData data{};
	data.resource = textureResource;
	data.cpuHandle = cpuHandle;
	data.gpuHandle = gpuHandle;
	data.index = srvIndex;

	textures_[filePath] = data;

	return srvIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandle(uint32_t index) {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	ID3D12DescriptorHeap* heap = dxCommon->GetSrvDescriptorHeap();
	UINT size = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += size * index;
	return handle;
}

ID3D12Resource* TextureManager::CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	// 1.metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);                             // Textureの幅
	resourceDesc.Height = UINT(metadata.height);                           // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);                   // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);            // 奥行き or 配列Textureの配列数
	resourceDesc.Format = metadata.format;                                 // Texture@Format
	resourceDesc.SampleDesc.Count = 1;                                     // サンプリングカウント。1固定。
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは2次元

	// 2.利用するHeapの設定。非常に特殊な運用。02_04exで一般的なケース版がある
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う

	// 3.Resourceを生成する
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                // Heapの設定
	    D3D12_HEAP_FLAG_NONE,           // Heapの特殊な設定。特になし。
	    &resourceDesc,                  // Resourceの設定
	    D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
	    nullptr,                        // Clear最適値。使わないのでnullptr
	    IID_PPV_ARGS(&resource));       // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}

ID3D12Resource* TextureManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12GraphicsCommandList* commandList) {

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// PrepareUploadを利用して、読み込んだデータからDirectX12用のSubresourceの配列を作成する
	// Subresourceは、MipMapの1枚1枚ぐらいのイメージ
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	// Subresourceの数を基に、コピー元となるIntermediateResourceに必要なサイズを計算する
	DirectX::PrepareUpload(dxCommon->GetDevice(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	// 計算したサイズでIntermediateResourceを作る
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	// CPUとGPUを取り持つためのResourceなので、IntermediateResource(中間リソース)と呼ぶ
	ID3D12Resource* intermediateResource = dxCommon->CreateBufferResource(intermediateSize);

	// ResourceStateを変更し、IntermediateResourceを返す
	// UpdateSubresourcesを利用して、IntermediateResourceにSubresourceのデータを書き込み、textureに転送するコマンドを積む
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
	// Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

} // namespace KujakuEngine