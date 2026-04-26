#pragma once

#include <array>
#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <wrl.h>

#pragma comment(lib, "dxcompiler.lib")

namespace KujakuEngine {

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

/// <summary>
/// グラフィックスパイプライン管理クラス
/// RootSignature・シェーダーコンパイル・PSOの生成を担当する
/// </summary>
class GraphicsPipeline {
public:
public:
	friend class Model;
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
	void SetCommandList(BlendMode blendMode);

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
	/// RootSignatureを生成する
	/// </summary>
	void CreateRootSignature();

	/// <summary>
	/// PSOを生成する
	/// </summary>
	void CreatePipelineStateObject();

private:
	// DXCコンパイラ関連
	IDxcUtils* dxcUtils_ = nullptr;
	IDxcCompiler3* dxcCompiler_ = nullptr;
	IDxcIncludeHandler* includeHandler_ = nullptr;

	// パイプライン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, static_cast<int32_t>(BlendMode::kCountOfBlendMode)> pipelineStates_;
};

} // namespace KujakuEngine