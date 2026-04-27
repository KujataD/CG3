#include "Particle.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include "DirectionalLight.h"
#include "GraphicsPipeline.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
namespace KujakuEngine {
void Particle::Initialize() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// Instancing用のTransformationMatrixリソースを作る
	instancingResource_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix) * kMaxInstance);

	// 書き込むためのアドレスを取得
	instancingResource_->Map(0, nullptr, (void**)&instancingData_);

	// Instancing用SRVを作成
	ID3D12DescriptorHeap* srvHeap = dxCommon->GetSrvDescriptorHeap();
	const UINT descriptorSizeSRV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	instancingSrvIndex_ = sInstancingSrvIndexCounter_++;
	instancingSrvHandleCPU_ = srvHeap->GetCPUDescriptorHandleForHeapStart();
	instancingSrvHandleCPU_.ptr += descriptorSizeSRV * instancingSrvIndex_;
	instancingSrvHandleGPU_ = srvHeap->GetGPUDescriptorHandleForHeapStart();
	instancingSrvHandleGPU_.ptr += descriptorSizeSRV * instancingSrvIndex_;

	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kMaxInstance;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(TransformationMatrix);

	dxCommon->GetDevice()->CreateShaderResourceView(instancingResource_.Get(), &instancingSrvDesc, instancingSrvHandleCPU_);

	// 単位行列を書き込んでおく
	for (uint32_t i = 0; i < kMaxInstance; ++i) {
		instancingData_[i].WVP = Matrix4x4::MakeIdentity();
		instancingData_[i].World = Matrix4x4::MakeIdentity();
	}
}
Particle* Particle::CreateFromOBJ(const std::string& objname, bool enableLighting) {
	Particle* particle = new Particle();
	std::string directoryPathFinal = "resources/" + objname;
	std::string filename = objname + ".obj";

	ModelData rawData = LoadObjFile(directoryPathFinal, filename);
	rawData.material.enableLighting = enableLighting;
	if (!rawData.material.textureFilePath.empty()) {
		rawData.material.textureIndex = TextureManager::GetInstance()->LoadTexture(rawData.material.textureFilePath);
	} else {
		rawData.material.textureIndex = TextureManager::GetInstance()->GetDefaultWhiteTexture();
	}
	particle->CreateVertexBuffer(rawData.vertices);
	particle->CreateMaterialBuffer(rawData.material);
	return particle;
}

Particle* Particle::CreateCube(const std::string& textureFilePath, bool enableLighting) {
	Particle* particle = new Particle();

	std::vector<VertexData> vertices = {
	    // 前面 (Z+)
	    {{-0.5f, 0.5f, 0.5f, 1.0f},   {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
	    {{-0.5f, -0.5f, 0.5f, 1.0f},  {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
	    {{0.5f, -0.5f, 0.5f, 1.0f},   {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
	    {{-0.5f, 0.5f, 0.5f, 1.0f},   {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
	    {{0.5f, -0.5f, 0.5f, 1.0f},   {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
	    {{0.5f, 0.5f, 0.5f, 1.0f},    {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },

	    // 背面 (Z-)
	    {{0.5f, 0.5f, -0.5f, 1.0f},   {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
	    {{0.5f, -0.5f, -0.5f, 1.0f},  {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
	    {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
	    {{0.5f, 0.5f, -0.5f, 1.0f},   {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
	    {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
	    {{-0.5f, 0.5f, -0.5f, 1.0f},  {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},

	    // 上面 (Y+)
	    {{-0.5f, 0.5f, -0.5f, 1.0f},  {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
	    {{-0.5f, 0.5f, 0.5f, 1.0f},   {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
	    {{0.5f, 0.5f, 0.5f, 1.0f},    {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
	    {{-0.5f, 0.5f, -0.5f, 1.0f},  {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
	    {{0.5f, 0.5f, 0.5f, 1.0f},    {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
	    {{0.5f, 0.5f, -0.5f, 1.0f},   {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },

	    // 下面 (Y-)
	    {{-0.5f, -0.5f, 0.5f, 1.0f},  {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
	    {{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
	    {{0.5f, -0.5f, -0.5f, 1.0f},  {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
	    {{-0.5f, -0.5f, 0.5f, 1.0f},  {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
	    {{0.5f, -0.5f, -0.5f, 1.0f},  {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
	    {{0.5f, -0.5f, 0.5f, 1.0f},   {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},

	    // 右面 (X+)
	    {{0.5f, 0.5f, 0.5f, 1.0f},    {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
	    {{0.5f, -0.5f, 0.5f, 1.0f},   {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
	    {{0.5f, -0.5f, -0.5f, 1.0f},  {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
	    {{0.5f, 0.5f, 0.5f, 1.0f},    {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
	    {{0.5f, -0.5f, -0.5f, 1.0f},  {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f} },
	    {{0.5f, 0.5f, -0.5f, 1.0f},   {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },

	    // 左面 (X-)
	    {{-0.5f, 0.5f, -0.5f, 1.0f},  {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
	    {{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
	    {{-0.5f, -0.5f, 0.5f, 1.0f},  {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
	    {{-0.5f, 0.5f, -0.5f, 1.0f},  {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
	    {{-0.5f, -0.5f, 0.5f, 1.0f},  {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
	    {{-0.5f, 0.5f, 0.5f, 1.0f},   {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
	};

	// MaterialData
	MaterialData defaultMaterial{};
	defaultMaterial.enableLighting = enableLighting;
	defaultMaterial.textureIndex = TextureManager::GetInstance()->LoadTexture(textureFilePath);

	particle->CreateVertexBuffer(vertices);
	particle->CreateMaterialBuffer(defaultMaterial);

	return particle;
}

Particle* Particle::CreatePlane(const std::string& textureFilePath, bool enableLighting) {
	Particle* particle = new Particle();

	std::vector<VertexData> vertices;

	vertices.push_back({
	    .position = {1.0f, 1.0f, 0.0f, 1.0f},
          .texcoord = {0.0f, 0.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    }); // 左上
	vertices.push_back({
	    .position = {-1.0f, 1.0f, 0.0f, 1.0f},
          .texcoord = {1.0f, 0.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    }); // 右上
	vertices.push_back({
	    .position = {1.0f, -1.0f, 0.0f, 1.0f},
          .texcoord = {0.0f, 1.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    }); // 左下
	vertices.push_back({
	    .position = {1.0f, -1.0f, 0.0f, 1.0f},
          .texcoord = {0.0f, 1.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    }); // 左下
	vertices.push_back({
	    .position = {-1.0f, 1.0f, 0.0f, 1.0f},
          .texcoord = {1.0f, 0.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    }); // 右上
	vertices.push_back({
	    .position = {-1.0f, -1.0f, 0.0f, 1.0f},
          .texcoord = {1.0f, 1.0f},
          .normal = {0.0f, 0.0f, 1.0f}
    }); // 右下

	// MaterialData
	MaterialData defaultMaterial{};
	defaultMaterial.enableLighting = enableLighting;
	defaultMaterial.textureIndex = TextureManager::GetInstance()->LoadTexture(textureFilePath);

	particle->CreateVertexBuffer(vertices);
	particle->CreateMaterialBuffer(defaultMaterial);

	return particle;
}

void Particle::PreDraw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	// 描画用のDescriptorHeapの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = {dxCommon->GetSrvDescriptorHeap()};
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	// ビューポートの設定
	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(WinApp::kWindowWidth);
	viewport.Height = static_cast<float>(WinApp::kWindowHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewport);

	// シザー矩形の設定
	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = WinApp::kWindowWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kWindowHeight;
	commandList->RSSetScissorRects(1, &scissorRect);

	// プリミティブトポロジの設定（main.cpp で毎フレームセットしているものに対応）
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Particle::PostDraw() {
	// 将来的にここで描画状態のリセットなどを行う
}

void Particle::Draw(const WorldTransform& worldTransform, const Camera& camera) {
	ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();

	// RootSignature と PSO をセット
	GraphicsPipeline::GetInstance()->SetCommandList(PipelineType::kParticle,blendMode_);

	// VBVを設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// マテリアルCBuffer（RootParameter[0]: PixelShader, b0）
	commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	// Instancing SRV（RootParameter[1]: VS t0 StructuredBuffer）
	commandList->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU_);
	
	// テクスチャSRV（RootParameter[2]: DescriptorTable）
	auto handle = TextureManager::GetInstance()->GetSrvHandle(textureIndex_);
	commandList->SetGraphicsRootDescriptorTable(2, handle);
	
	// ライト
	commandList->SetGraphicsRootConstantBufferView(3, DirectionalLight::GetInstance()->GetResource()->GetGPUVirtualAddress());
	// 描画
	commandList->DrawInstanced(vertexCount_, static_cast<uint32_t>(instanceTransforms_.size()), 0, 0);
	instanceTransforms_.clear();
}

void Particle::UpdateBuffer() { memcpy(instancingData_, instanceTransforms_.data(), sizeof(TransformationMatrix) * instanceTransforms_.size()); }

MaterialData Particle::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// ファイル名だけ取り出す（絶対パス対策）
			std::filesystem::path texPath(textureFilename);
			materialData.textureFilePath = directoryPath + "/" + texPath.filename().string();
		}
	}
	return materialData;
}

ModelData Particle::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData ModelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;
	std::string line;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			position.x *= -1.0f;
			positions.push_back(position);
		} else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f;
			normals.push_back(normal);
		} else if (identifier == "f") {
			// 頂点数を動的に取得する
			std::vector<VertexData> faceVertices;
			std::string vertexDefinition;

			while (s >> vertexDefinition) {
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}
				faceVertices.push_back({positions[elementIndices[0] - 1], texcoords[elementIndices[1] - 1], normals[elementIndices[2] - 1]});
			}

			// 三角形に分割
			for (size_t i = 1; i + 1 < faceVertices.size(); ++i) {
				ModelData.vertices.push_back(faceVertices[0]);
				ModelData.vertices.push_back(faceVertices[i + 1]);
				ModelData.vertices.push_back(faceVertices[i]);
			}
		} else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			ModelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return ModelData;
}

void Particle::CreateVertexBuffer(const std::vector<VertexData>& vertices) {
	vertexCount_ = static_cast<uint32_t>(vertices.size());

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = sizeof(VertexData) * vertexCount_;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource_));
	assert(SUCCEEDED(hr));

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertexCount_);
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	VertexData* vertexMap = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap));
	std::memcpy(vertexMap, vertices.data(), sizeof(VertexData) * vertexCount_);
	vertexResource_->Unmap(0, nullptr);
}

void Particle::CreateMaterialBuffer(const MaterialData& material) {
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = sizeof(MaterialData);
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&materialResource_));
	assert(SUCCEEDED(hr));

	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialMap_));
	materialMap_->color = material.color;
	materialMap_->enableLighting = material.enableLighting;
	materialMap_->uvTransform = Matrix4x4::MakeIdentity();
	textureIndex_ = material.textureIndex;
}
} // namespace KujakuEngine
