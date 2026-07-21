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
	// 色/テクスチャ/シェーダーは全サブメッシュへ一括適用する(方式B: Material AssL一括上書き)。
	void SetColor(const Vector4& color) {
		for (SubMesh& subMesh : subMeshes_) {
			if (subMesh.materialMap) {
				subMesh.materialMap->color = color;
			}
		}
	}
	void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
	void SetTexture(uint32_t textureIndex) {
		for (SubMesh& subMesh : subMeshes_) {
			subMesh.textureIndex = textureIndex;
		}
	}
	// シェーダー方式を切り替える(MaterialData.enableLightingがShaderModel番号を兼ねる。0=Unlit)。
	void SetShaderModel(ShaderModel model) {
		for (SubMesh& subMesh : subMeshes_) {
			if (subMesh.materialMap) {
				subMesh.materialMap->enableLighting = static_cast<int32_t>(model);
			}
		}
	}

	// --- get ---

	// raycast/preview用: 全サブメッシュを統合した頂点列(AABB計算に使う)。
	const std::vector<VertexData>& GetVertices() const { return mergedVertices_; }

	// サブメッシュ数。>1 ならMultiMesh(埋め込みマテリアルをパーツ別に持つ)。
	size_t GetSubMeshCount() const { return subMeshes_.size(); }

	const Matrix4x4& GetRootLocalMatrix() const { return rootLocalMatrix_; }

private:
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	// 1サブメッシュ分のGPUリソース(頂点バッファ + マテリアルCBuffer + テクスチャ)。
	struct SubMesh {
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
		uint32_t vertexCount = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
		MaterialData* materialMap = nullptr;
		uint32_t textureIndex = 0;
	};

	std::vector<SubMesh> subMeshes_;
	// 全サブメッシュを統合した頂点(GetVertices用)。
	std::vector<VertexData> mergedVertices_;
	Matrix4x4 rootLocalMatrix_ = MakeIdentity();

	// 頂点とマテリアルから1サブメッシュ分のGPUリソースを生成し、subMeshes_へ追加する。
	void AddSubMesh(const std::vector<VertexData>& vertices, const MaterialData& material);

	// ModelDataのサブメッシュ(空なら統合vertices/materialでフォールバック)から
	// サブメッシュ群を構築する。各マテリアルにshaderModelとテクスチャindexを解決して設定する。
	void BuildSubMeshes(ModelData& modelData, ShaderModel shaderModel);

	BlendMode blendMode_ = BlendMode::kNormal;
};

} // namespace KujakuEngine
