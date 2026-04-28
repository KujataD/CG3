#pragma once

#include <array>
#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <vector>
#include <wrl.h>

#pragma comment(lib, "dxcompiler.lib")

#include "../math/Matrix4x4.h"
#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"

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
	float shininess = 40.0f;
	std::string textureFilePath;
	uint32_t textureIndex;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

enum class BlendMode {
	kNone,      // ブレンドなし
	kNormal,    // αブレンド	: Src * SrcA + Dest * (1 - SrcA)
	kAdd,       // 加算		: Src * SrcA + Dest * 1
	kSubtract,  // 減算		: Dest * 1 - Src * SrcA
	kMultiply,  // 乗算		: Src * 0 + Dest * Src
	kScreen,    // スクリーン	: Src * (1 - Dest) + Dest * 1
	kExclusion, // 除外		: (1 - Dest) * Src + (1 - Src) * Dest

	kCountOfBlendMode, //!< ブレンドモード数。指定はしない
};

enum class PipelineType {
	kObject3d,
	kParticle,
	kCountOfPipeLineType,
};

enum class ShaderModel {
	kNone,                 // シェーダーなし
	kLambert,              // ランバート
	kHalfLambert,          // ハーフランバート
	kPhongReflection,      // フォンリフレクション
	kBlingPhongReflection, // ブリンフォンリフレクション
};

/// <summary>
/// グラフィックスパイプライン管理クラス
/// RootSignature・シェーダーコンパイル・PSOの生成を担当する
/// </summary>
class GraphicsPipeline {
public:
public:
	friend class Model;
	friend class ParticleModel;
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static GraphicsPipeline* GetInstance();

	/// <summary>
	/// 初期化
	/// RootSignature・シェーダー・PSOを生成する
	/// </summary>
	void Initialize();

	/// <summary>
	/// 描画前コマンドのセット
	/// RootSignature・PSO・DescriptorHeap・Viewport・ScissorRect をコマンドリストに積む
	/// </summary>
	void SetCommandList(PipelineType pipelineType, BlendMode blendMode);

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

private:
	GraphicsPipeline() = default;
	~GraphicsPipeline() = default;
	GraphicsPipeline(const GraphicsPipeline&) = delete;
	GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

	/// <summary>
	/// DXCコンパイラの初期化
	/// </summary>
	void InitializeDXC();

	/// <summary>
	/// シェーダーをコンパイルする
	/// </summary>
	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);

	/// <summary>
	/// Object3d用RootSignatureを生成する
	/// </summary>
	void CreateObject3dRootSignature();

	/// <summary>
	/// Particle用RootSignatureを生成する
	/// </summary>
	void CreateParticleRootSignature();

	/// <summary>
	/// Object3d用PSOを生成する
	/// </summary>
	void CreateObject3dPipelineStateObject();

	/// <summary>
	/// Sprite用PSOを生成する
	/// </summary>
	void CreateParticlePipelineStateObject();

private:
	// DXCコンパイラ関連
	IDxcUtils* dxcUtils_ = nullptr;
	IDxcCompiler3* dxcCompiler_ = nullptr;
	IDxcIncludeHandler* includeHandler_ = nullptr;

	// パイプライン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_[static_cast<int32_t>(PipelineType::kCountOfPipeLineType)];
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStates_[static_cast<int32_t>(PipelineType::kCountOfPipeLineType)][static_cast<int32_t>(BlendMode::kCountOfBlendMode)];
};

} // namespace KujakuEngine