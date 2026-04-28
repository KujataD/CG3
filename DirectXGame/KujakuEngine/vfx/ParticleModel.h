#pragma once

#include "../3d/Camera.h"
#include "../3d/GraphicsPipeline.h"
#include "../3d/WorldTransform.h"
#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "../../externals/DirectXTex/DirectXTex.h"

namespace KujakuEngine {

struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

/// <summary>
/// 3Dモデル
/// </summary>
class ParticleModel {
public:
	ParticleModel() = default;
	~ParticleModel();

	void Initialize();

	/// <summary>
	/// OBJファイルからモデルを生成する(省略版)
	/// </summary>
	static ParticleModel* CreateFromOBJ(const std::string& objname, bool enableLighting = false);

	static ParticleModel* CreateCube(const std::string& textureFilePath, bool enableLighting = false);

	static ParticleModel* CreatePlane(const std::string& textureFilePath, bool enableLighting);

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
	void Draw();

	void UpdateBuffer();

	// --- set ---
	void SetColor(const Vector4& color) { materialMap_->color = color; }
	void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
	bool AddInstanceParticle(const TransformationMatrix& transformationMatrix, const Vector4& color) {
		// 最大値を超えたらプッシュしない
		if (static_cast<uint32_t>(instanceParticles_.size()) >= kMaxInstance) {
			return false;
		}
		TransformationMatrix transformationMat = transformationMatrix;
		ParticleForGPU particleForGPU = {transformationMat.WVP, transformationMat.World, color};
		instanceParticles_.push_back(particleForGPU);
		return true;
	}

	// --- get ---
	uint32_t GetMaxInstance() const { return kMaxInstance; }
	ID3D12Resource* GetInstancingResource() { return instancingResource_.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	uint32_t vertexCount_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	MaterialData* materialMap_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	ParticleForGPU* instancingData_;
	std::vector<ParticleForGPU> instanceParticles_;

	BlendMode blendMode_ = BlendMode::kNormal;

	uint32_t textureIndex_;

	static inline const uint32_t kMaxInstance = 1000;

	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_{};
	uint32_t instancingSrvIndex_ = 0;
	static inline uint32_t sInstancingSrvIndexCounter_ = 64;

	ParticleModel(const ParticleModel&) = delete;
	ParticleModel& operator=(const ParticleModel&) = delete;

	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	void CreateVertexBuffer(const std::vector<VertexData>& vertices);
	void CreateMaterialBuffer(const MaterialData& material);
};

} // namespace KujakuEngine

