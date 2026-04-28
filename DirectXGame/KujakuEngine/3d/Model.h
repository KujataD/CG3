#pragma once

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "../3d/Camera.h"
#include "../3d/WorldTransform.h"
#include "../3d/GraphicsPipeline.h"
#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"

#include "../../externals/DirectXTex/DirectXTex.h"

namespace KujakuEngine {


/// <summary>
/// 3Dモデル
/// </summary>
class Model {
public:
	Model() = default;
	~Model() = default;
	
	/// <summary>
	/// OBJファイルからモデルを生成する(省略版)
	/// </summary>
	static Model* CreateFromOBJ(const std::string& objname, bool enableLighting = false);

	static Model* CreateSphere(const std::string& textureFilePath, bool enableLighting = false, uint32_t subdivision = 16);
	 
	static Model* CreateCube(const std::string& textureFilePath, bool enableLighting = false);

	static Model* CreatePlane(const std::string& textureFilePath, bool enableLighting = false);

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
	void SetBlendMode(BlendMode mode) { blendMode_ = mode; }

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	uint32_t vertexCount_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	MaterialData* materialMap_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

	uint32_t textureIndex_;

	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	void CreateVertexBuffer(const std::vector<VertexData>& vertices);
	void CreateMaterialBuffer(const MaterialData& material);

	BlendMode blendMode_ = BlendMode::kNormal;
};

} // namespace KujakuEngine