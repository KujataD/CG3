#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <wrl.h>

#pragma comment(lib, "dxcompiler.lib")

namespace KujakuEngine {

/// <summary>
/// グラフィックスパイプライン管理クラス
/// RootSignature・シェーダーコンパイル・PSOの生成を担当する
/// </summary>
class GraphicsPipeline {
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
	void SetCommandList();

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
};

} // namespace KujakuEngine