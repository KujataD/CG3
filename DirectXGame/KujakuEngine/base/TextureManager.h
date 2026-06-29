#pragma once
#include <cstdint>
#include <d3d12.h>
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <wrl.h>

#include "../3d/Model.h" // VertexData, MaterialData を共有

#include "../../externals/DirectXTex/DirectXTex.h"
#include "../../externals/DirectXTex/d3dx12.h"
#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"

namespace KujakuEngine {

class TextureManager {
public:
	struct TextureLoadEvent {
		std::string filePath;
		float loadMs = 0.0f;
		bool cacheHit = false;
	};

	struct TextureData {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		uint32_t index;
	};

	void Initialize();

	static TextureManager* GetInstance();

	/// <summary>
	/// テクスチャを読み込む
	/// </summary>
	/// <param name="filePath"></param>
	uint32_t LoadTexture(const std::string& filePath);

	/// <summary>
	/// シェーダーリソースビューハンドル取得
	/// </summary>
	/// <param name="index"></param>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(uint32_t index);
	const std::deque<TextureLoadEvent>& GetRecentLoadEvents() const { return recentLoadEvents_; }

	uint32_t GetDefaultWhiteTexture() const { return defaultWhiteTextureIndex_; }

private:
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager&) = delete;
	const TextureManager& operator=(const TextureManager&) = delete;

	ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);
	
	// nodiscard : 戻り値を破棄してはいけない（破棄でエラー）
	[[nodiscard]]
	ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12GraphicsCommandList* commandList);

private:
	// srvのインデックス管理用（Imgui用に0は空けておく）
	uint32_t srvIndexCounter_ = 1;
	uint32_t defaultWhiteTextureIndex_;

	// テクスチャコンテナ
	std::unordered_map<std::string, TextureData> textures_;
	std::deque<TextureLoadEvent> recentLoadEvents_;
};

} // namespace KujakuEngine