#pragma once

#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "../3d/Camera.h"
#include "../3d/GraphicsPipeline.h"
#include "../3d/WorldTransform.h"
#include "../runtime/KujakuApi.h"
#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"

#include "../../externals/DirectXTex/DirectXTex.h"

namespace KujakuEngine {

enum FillMode { kFillModeSolid, kFillModeWireframe };

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
	static KUJAKU_API Model* CreateFromOBJ(const std::string& objname, ShaderModel shaderModel = ShaderModel::kNone);
	static KUJAKU_API Model* CreateFromGlTF(const std::string& objname, ShaderModel shaderModel = ShaderModel::kNone);
	static KUJAKU_API Model* TryCreateFromFile(const std::string& filePath, ShaderModel shaderModel = ShaderModel::kNone);

	static KUJAKU_API Model* CreateSphere(const std::string& textureFilePath, ShaderModel shaderModel = ShaderModel::kNone, uint32_t subdivision = 16);

	static KUJAKU_API Model* CreateCube(const std::string& textureFilePath, ShaderModel shaderModel = ShaderModel::kNone);

	/// <summary>
	/// カプセル(cylinder + 両端半球)のテンプレモデルを生成する。
	/// heightは両端の半球を含む全長で、cylinder部分の長さは height - 2*radius。
	/// </summary>
	static KUJAKU_API Model* CreateCapsule(const std::string& textureFilePath, ShaderModel shaderModel = ShaderModel::kNone, float radius = 0.5f, float height = 2.0f, uint32_t subdivision = 16);

	static KUJAKU_API Model* CreatePlane(const std::string& textureFilePath, ShaderModel shaderModel = ShaderModel::kNone);

	static KUJAKU_API Model* CreateTriangle(const std::string& textureFilePath, ShaderModel shaderModel = ShaderModel::kNone);

	static KUJAKU_API Model* CreateTetrahedron(const std::string& textureFilePath, ShaderModel shaderModel = ShaderModel::kNone);

	/// <summary>
	/// 描画前処理（全モデル共通・フレームに1回）
	/// RootSignature / PSO / Viewport / ScissorRect / PrimitiveTopology をセットする
	/// main.cpp のループ内「描画用設定」に対応
	/// </summary>
	static KUJAKU_API void PreDraw();

	/// <summary>
	/// 描画後処理（将来の拡張のために用意）
	/// </summary>
	static KUJAKU_API void PostDraw();

	/// <summary>
	/// 描画（PreDraw の後に呼ぶ）
	/// </summary>
	KUJAKU_API void Draw(const WorldTransform& worldTransform, const Camera& camera, FillMode fillMode = kFillModeSolid);

	// --- set ---
	void SetColor(const Vector4& color) {
		if (materialMap_) {
			materialMap_->color = color;
		}
	}
	void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
	void SetTexture(uint32_t textureIndex) { textureIndex_ = textureIndex; }

	// --- get ---

	const std::vector<VertexData>& GetVertices() const { return vertices_; }

	const Matrix4x4& GetRootLocalMatrix() const { return rootLocalMatrix_; }

private:
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	uint32_t vertexCount_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	MaterialData* materialMap_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

	// モデル頂点
	std::vector<VertexData> vertices_;
	Matrix4x4 rootLocalMatrix_ = MakeIdentity();

	uint32_t textureIndex_ = 0;
	void CreateVertexBuffer(const std::vector<VertexData>& vertices);
	void CreateMaterialBuffer(const MaterialData& material);

	BlendMode blendMode_ = BlendMode::kNormal;
};

} // namespace KujakuEngine
