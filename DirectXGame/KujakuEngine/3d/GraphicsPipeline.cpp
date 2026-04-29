#include "GraphicsPipeline.h"
#include "../base/DirectXCommon.h"
#include "../base/Logger.h"
#include "../base/StringUtil.h"
#include "../base/WinApp.h"
#include <cassert>
#include <format>

namespace KujakuEngine {

GraphicsPipeline* GraphicsPipeline::GetInstance() {
	static GraphicsPipeline instance;
	return &instance;
}

void GraphicsPipeline::Initialize() {
	InitializeDXC();
	CreateObject3dRootSignature();
	CreateParticleRootSignature();
	CreateObject3dPipelineStateObject();
	CreateParticlePipelineStateObject();
}

void GraphicsPipeline::InitializeDXC() {
	HRESULT hr;

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));

	// includeに対応するための設定
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}

IDxcBlob* GraphicsPipeline::CompileShader(const std::wstring& filePath, const wchar_t* profile) {
	// シェーダーコンパイルする旨をログに出す
	OutputDebugStringW(std::format(L"Begin CompileShader, path: {}, profile: {}\n", filePath, profile).c_str());

	// hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	// コンパイルオプション
	LPCWSTR arguments[] = {
	    filePath.c_str(), // コンパイル対象のhlslファイル名
	    L"-E",
	    L"main", // エントリーポイントの指定
	    L"-T",
	    profile, // ShaderProfileの設定
	    L"-Zi",
	    L"-Qembed_debug", // デバッグ用の情報を埋め込む
	    L"-Od",           // 最適化を外しておく
	    L"-Zpr",          // メモリレイアウトは行優先
	};

	// 実際にShaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler_->Compile(
	    &shaderSourceBuffer,        // 読み込んだファイル
	    arguments,                  // コンパイルオプション
	    _countof(arguments),        // コンパイルオプションの数
	    includeHandler_,            // includeが含まれた諸々
	    IID_PPV_ARGS(&shaderResult) // コンパイル結果
	);

	// コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	// 3. 警告·エラーがでていないか確認する

	// 警告·エラーが出てたらログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Logger::Log(shaderError->GetStringPointer());
		// 警告·エラーダメゼッタイ
		assert(false);
	}

	// 4. Compile結果を受け取って返す

	// コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	// 成功したログを出す
	Logger::Log(StringUtil::ToString(std::format(L"Compile Succeeded, path: {}, profile: {}\n", filePath, profile)));
	// もう使わないリソースを解放
	shaderSource->Release();
	shaderResult->Release();
	// 実行用のバイナリを返却
	return shaderBlob;
}

void GraphicsPipeline::CreateObject3dRootSignature() {
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr;

	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// RootParameter作成
	// b0 Material
	D3D12_ROOT_PARAMETER rootParameters[7] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う b0のbに対応する bはConstantBuffer
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;                    // レジスタ番号0とバインド b0の0に対応する。もしb11と紐づけたいなら11となる。

	// b0 TransformationMatrix
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0;                     // レジスタ番号0を使う

	// DescriptorRange 複数のDescriptorの設定を一括で行う
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;                                                   // 0から始まる
	descriptorRange[0].NumDescriptors = 1;                                                       // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                              // SRVを使う(t)
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

	// t0 Texture
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;      // DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;             // Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

	// b1 Directional Light
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1;                    // レジスタ番号1を使う

	// b2 Camera
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[4].Descriptor.ShaderRegister = 2;                    // レジスタ番号2を使う

	// b3 ポイントライト
	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[5].Descriptor.ShaderRegister = 3;                    // レジスタ番号3を使う

	// b4 スポットライト
	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[6].Descriptor.ShaderRegister = 4;                    // レジスタ番号4を使う

	descriptionRootSignature.pParameters = rootParameters;              // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ

	// Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;   // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;     // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;                       // ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0;                               // レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	// バイナリを元に生成
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_[static_cast<int32_t>(PipelineType::kObject3d)]));
	assert(SUCCEEDED(hr));

	signatureBlob->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
}

void GraphicsPipeline::CreateParticleRootSignature() {
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr;

	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// DescriptorRange 複数のDescriptorの設定を一括で行う
	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
	descriptorRangeForInstancing[0].BaseShaderRegister = 0;                                                   // 0から始まる
	descriptorRangeForInstancing[0].NumDescriptors = 1;                                                       // 数は1つ
	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                              // SRVを使う
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

	// RootParameter作成。複数設定できるので配列。今回は結果1つだけなので長さ1の配列
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う b0のbに対応する bはConstantBuffer
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;                    // レジスタ番号0とバインド b0の0に対応する。もしb11と紐づけたいなら11となる。

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;                   // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;                            // VertexShaderで使う
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;             //  Tableの中身の配列を指定
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing); // Tableで利用する数

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;                   // DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                             // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;             // Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing); // Tableで利用する数

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1;                    // レジスタ番号1を使う
	descriptionRootSignature.pParameters = rootParameters;              // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ

	// Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;   // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;     // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;                       // ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0;                               // レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	// バイナリを元に生成
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_[static_cast<int32_t>(PipelineType::kParticle)]));
	assert(SUCCEEDED(hr));

	signatureBlob->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
}

void GraphicsPipeline::CreateObject3dPipelineStateObject() {
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();

	// シェーダーをコンパイルする
	IDxcBlob* vertexShaderBlob = CompileShader(L"KujakuEngine/shader/Object3D.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = CompileShader(L"KujakuEngine/shader/Object3D.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// 2. InputLayoutの設定
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// 3. BlendStateの設定（すべての色要素を書き込む）
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// 4. RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;  // 裏面（時計回り）を表示しない
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 三角形の中を塗りつぶす

	// 7. DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSOの生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_[static_cast<int32_t>(PipelineType::kObject3d)].Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	for (int32_t i = 0; i < static_cast<int32_t>(BlendMode::kCountOfBlendMode); i++) {
		D3D12_BLEND_DESC blendDesc{};
		auto& renderTarget = blendDesc.RenderTarget[0];

		// 共通初期化部
		renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		renderTarget.BlendEnable = TRUE;

		renderTarget.SrcBlend = D3D12_BLEND_ONE;
		renderTarget.DestBlend = D3D12_BLEND_ZERO;
		renderTarget.BlendOp = D3D12_BLEND_OP_ADD;

		renderTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
		renderTarget.DestBlendAlpha = D3D12_BLEND_ZERO;
		renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		switch (static_cast<BlendMode>(i)) {
		case BlendMode::kNone:
			renderTarget.BlendEnable = FALSE;
			break;

		case BlendMode::kNormal:
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			break;

		case BlendMode::kAdd:
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.DestBlend = D3D12_BLEND_ONE;
			break;

		case BlendMode::kMultiply:
			renderTarget.SrcBlend = D3D12_BLEND_ZERO;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.DestBlend = D3D12_BLEND_SRC_COLOR;
			break;

		case BlendMode::kExclusion:
			renderTarget.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			renderTarget.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::kScreen:
			renderTarget.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.DestBlend = D3D12_BLEND_ONE;
			break;

		case BlendMode::kSubtract:
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			renderTarget.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			renderTarget.DestBlend = D3D12_BLEND_ONE;
			break;

		default:
			break;
		}

		graphicsPipelineStateDesc.BlendState = blendDesc;
		HRESULT hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStates_[static_cast<int32_t>(PipelineType::kObject3d)][i]));
		assert(SUCCEEDED(hr));
	}

	vertexShaderBlob->Release();
	pixelShaderBlob->Release();
}

void GraphicsPipeline::CreateParticlePipelineStateObject() {
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();

	// シェーダーをコンパイルする
	IDxcBlob* vertexShaderBlob = CompileShader(L"KujakuEngine/shader/Particle.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = CompileShader(L"KujakuEngine/shader/Particle.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// 2. InputLayoutの設定
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// 3. BlendStateの設定（すべての色要素を書き込む）
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// 4. RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;  // 裏面（時計回り）を表示しない
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // 三角形の中を塗りつぶす

	// 7. DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // Depthの書き込みを行わない
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSOの生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_[static_cast<int32_t>(PipelineType::kParticle)].Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	for (int32_t i = 0; i < static_cast<int32_t>(BlendMode::kCountOfBlendMode); i++) {
		D3D12_BLEND_DESC blendDesc{};
		auto& renderTarget = blendDesc.RenderTarget[0];

		// 共通初期化部
		renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		renderTarget.BlendEnable = TRUE;

		renderTarget.SrcBlend = D3D12_BLEND_ONE;
		renderTarget.DestBlend = D3D12_BLEND_ZERO;
		renderTarget.BlendOp = D3D12_BLEND_OP_ADD;

		renderTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
		renderTarget.DestBlendAlpha = D3D12_BLEND_ZERO;
		renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		switch (static_cast<BlendMode>(i)) {
		case BlendMode::kNone:
			renderTarget.BlendEnable = FALSE;
			break;

		case BlendMode::kNormal:
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			break;

		case BlendMode::kAdd:
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.DestBlend = D3D12_BLEND_ONE;
			break;

		case BlendMode::kMultiply:
			renderTarget.SrcBlend = D3D12_BLEND_ZERO;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.DestBlend = D3D12_BLEND_SRC_COLOR;
			break;

		case BlendMode::kExclusion:
			renderTarget.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			renderTarget.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::kScreen:
			renderTarget.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.DestBlend = D3D12_BLEND_ONE;
			break;

		case BlendMode::kSubtract:
			renderTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			renderTarget.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			renderTarget.DestBlend = D3D12_BLEND_ONE;
			break;

		default:
			break;
		}

		graphicsPipelineStateDesc.BlendState = blendDesc;
		HRESULT hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStates_[static_cast<int32_t>(PipelineType::kParticle)][i]));
		assert(SUCCEEDED(hr));
	}

	vertexShaderBlob->Release();
	pixelShaderBlob->Release();
}

void GraphicsPipeline::SetCommandList(PipelineType pipelineType, BlendMode blendMode) {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	// RootSignatureとPSOを設定
	commandList->SetGraphicsRootSignature(rootSignature_[static_cast<int32_t>(pipelineType)].Get());
	commandList->SetPipelineState(pipelineStates_[static_cast<int32_t>(pipelineType)][static_cast<int32_t>(blendMode)].Get());
}

void GraphicsPipeline::Finalize() {
	if (includeHandler_) {
		includeHandler_->Release();
		includeHandler_ = nullptr;
	}
	if (dxcCompiler_) {
		dxcCompiler_->Release();
		dxcCompiler_ = nullptr;
	}
	if (dxcUtils_) {
		dxcUtils_->Release();
		dxcUtils_ = nullptr;
	}
}

} // namespace KujakuEngine