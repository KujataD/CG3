#pragma once
#include <cstdint>
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <map>

#include "../3d/Model.h" // VertexData, MaterialData を共有

#include "../../externals/DirectXTex/DirectXTex.h"
#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"

namespace KujakuEngine {

class TextureManager {
public:
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

	/// <summary>
	/// srvIndexを割り当てます
	/// </summary>
	/// <returns></returns>
	//uint32_t AllocateSrvIndex() { return srvIndexCounter_++; }

	uint32_t GetDefaultWhiteTexture() const { return defaultWhiteTextureIndex_; }

private:
	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager&) = delete;
	const TextureManager& operator=(const TextureManager&) = delete;

	// srvのインデックス管理用（Imgui用に0は空けておく）
	uint32_t srvIndexCounter_ = 1;
	uint32_t defaultWhiteTextureIndex_;
	
	// テクスチャコンテナ
	std::unordered_map<std::string, TextureData> textures_;
};

} // namespace KujakuEngine