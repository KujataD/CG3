#pragma once

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "../3d/Camera.h"
#include "../3d/WorldTransform.h"
#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"

#include "../../externals/DirectXTex/DirectXTex.h"

namespace KujakuEngine {

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct MaterialData {
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	int32_t enableLighting = 0;
	float pad[3] = {};
	Matrix4x4 uvTransform;
	std::string textureFilePath;
};

struct ModelRawData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

/// <summary>
/// 3Dモデル
/// </summary>
class Model {
public:
	Model() = default;
	~Model() = default;
	
	/// <summary>
	/// OBJファイルからモデルを生成する
	/// </summary>
	Model* CreateFromOBJ(const std::string& directoryPath, const std::string& filename, bool enableLighting);

	/// <summary>
	/// OBJファイルからモデルを生成する(省略版)
	/// </summary>
	static Model* CreateFromOBJ(const std::string& objname, bool enableLighting = false);

	static Model* Create(const std::string& textureFilePath, bool enableLighting = false);


	/// <summary>
	/// 描画前処理（全モデル共通・フレームに1回）
	/// RootSignature / PSO / Viewport / ScissorRect / PrimitiveTopology をセットする
	/// main.cpp のループ内「描画用設定」に対応
	/// </summary>
	static void PreDraw();

	/// <summary>
	/// 描画後処理（将来の拡張のために用意）
	/// </summary>
	static void PostDraw();

	/// <summary>
	/// 描画（PreDraw の後に呼ぶ）
	/// </summary>
	void Draw(const WorldTransform& worldTransform, const Camera& camera);


	// --- set ---
	void SetColor(const Vector4& color) { materialMap_->color = color; }

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	uint32_t vertexCount_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	MaterialData* materialMap_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	static ModelRawData LoadObjFile(const std::string& directoryPath, const std::string& filename);
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	void LoadTexture(const std::string& filePath);
	void CreateVertexBuffer(const std::vector<VertexData>& vertices);
	void CreateMaterialBuffer(const MaterialData& material);
};

} // namespace KujakuEngine