#include "InstancingModel.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include "../3d/DirectionalLight.h"
#include "../3d/GraphicsPipeline.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace KujakuEngine {

InstancingModel::~InstancingModel() { instanceParticles_.clear(); }

void InstancingModel::Initialize() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// Instancing用のTransformationMatrixリソースを作る
	instancingResource_ = dxCommon->CreateBufferResource(sizeof(ParticleForGPU) * kMaxInstance);

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
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	dxCommon->GetDevice()->CreateShaderResourceView(instancingResource_.Get(), &instancingSrvDesc, instancingSrvHandleCPU_);
		
	// 毎フレーム push_back するため、先に最大数まで予約して再確保スパイクを防ぐ
	instanceParticles_.reserve(kMaxInstance);

	// 単位行列を書き込んでおく
	for (uint32_t i = 0; i < kMaxInstance; ++i) {
		instancingData_[i].WVP = MakeIdentity();
		instancingData_[i].World = MakeIdentity();
	}
}

InstancingModel* InstancingModel::CreateFromOBJ(const std::string& objname, bool enableLighting) {
	InstancingModel* particle = new InstancingModel();
	std::string directoryPathFinal = "Resources/" + objname;
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
	particle->Initialize();
	return particle;
}

InstancingModel* InstancingModel::CreateCube(const std::string& textureFilePath, bool enableLighting) {
	InstancingModel* particle = new InstancingModel();

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
	particle->Initialize();
	return particle;
}

InstancingModel* InstancingModel::CreatePlane(const std::string& textureFilePath, bool enableLighting) {
	InstancingModel* particle = new InstancingModel();

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
	particle->Initialize();
	return particle;
}

InstancingModel* InstancingModel::CreateTetrahedron(const std::string& textureFilePath, bool enableLighting) {
	InstancingModel* particle = new InstancingModel();

	std::vector<VertexData> vertices;

	const Vector3 v0 = { 1.0f,  1.0f,  1.0f };
	const Vector3 v1 = { -1.0f, -1.0f,  1.0f };
	const Vector3 v2 = { -1.0f,  1.0f, -1.0f };
	const Vector3 v3 = { 1.0f, -1.0f, -1.0f };

	auto AddFace = [&](const Vector3& a, const Vector3& b, const Vector3& c) {

		Vector3 normal =
			Normalize(Cross(b - a, c - a));

		vertices.push_back({
			.position = {a.x, a.y, a.z, 1.0f},
			.texcoord = {0.5f, 0.0f},
			.normal = normal
			});

		vertices.push_back({
			.position = {b.x, b.y, b.z, 1.0f},
			.texcoord = {0.0f, 1.0f},
			.normal = normal
			});

		vertices.push_back({
			.position = {c.x, c.y, c.z, 1.0f},
			.texcoord = {1.0f, 1.0f},
			.normal = normal
			});
		};

	// 4面
	AddFace(v0, v2, v1);
	AddFace(v0, v1, v3);
	AddFace(v0, v3, v2);
	AddFace(v1, v2, v3);

	// MaterialData
	MaterialData defaultMaterial{};
	defaultMaterial.enableLighting = enableLighting;
	defaultMaterial.textureIndex =
		TextureManager::GetInstance()->LoadTexture(textureFilePath);

	particle->CreateVertexBuffer(vertices);
	particle->CreateMaterialBuffer(defaultMaterial);
	particle->Initialize();

	return particle;
}

void InstancingModel::PreDraw() {
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

void InstancingModel::PostDraw() {
	// 将来的にここで描画状態のリセットなどを行う
}

void InstancingModel::Draw() {
	ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();

	// RootSignature と PSO をセット
	GraphicsPipeline::GetInstance()->SetCommandList(PipelineType::kInstancingObject3d, blendMode_);

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
	commandList->DrawInstanced(vertexCount_, static_cast<uint32_t>(instanceParticles_.size()), 0, 0);
	instanceParticles_.clear();
}

void InstancingModel::UpdateBuffer() { memcpy(instancingData_, instanceParticles_.data(), sizeof(ParticleForGPU) * instanceParticles_.size()); }

MaterialData InstancingModel::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
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

ModelData InstancingModel::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;

	// Assimpをつかってobjファイルを読む
	// ------------------------------------------
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	// aiProcess_FlipWindingOrder <! 三角形の並び順を逆にする。
	// aiProcess_Triangulate <! 四角形以上の面を三角形に分割する。
	// aiProcess_FlipUVs !< UVをフリップする。
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_FlipUVs);
	assert(scene);
	assert(scene->HasMeshes()); // メッシュがないのは対応しない

	// meshを解析する
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());// 法線がないMeshは今回は非対応
		assert(mesh->HasTextureCoords(0));// TexcoordがないMeshは今回は非対応

		// faceを解析する
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);// 三角形のみサポート

			// vertexを解析する
			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
				VertexData vertex;
				vertex.position = { position.x, position.y, position.z, 1.0f };
				vertex.normal = { normal.x, normal.y, normal.z };
				vertex.texcoord = { texcoord.x, texcoord.y };
				//aiProcess_MakeLeftHandedはz*=-1で、右手->左手に変換するので手動で対処
				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;
				modelData.vertices.push_back(vertex);

			}
		}
	}

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
		aiMaterial* material = scene->mMaterials[materialIndex];

		// aiTextureType_DIFFUSE <! テクスチャを模様として利用する
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
		}
	}

	return modelData;
}

void InstancingModel::CreateVertexBuffer(const std::vector<VertexData>& vertices) {
	vertexCount_ = static_cast<uint32_t>(vertices.size());
	vertexResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * vertexCount_);

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertexCount_);
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	VertexData* vertexMap = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap));
	std::memcpy(vertexMap, vertices.data(), sizeof(VertexData) * vertexCount_);
	vertexResource_->Unmap(0, nullptr);
}

void InstancingModel::CreateMaterialBuffer(const MaterialData& material) {
	materialResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(MaterialData));

	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialMap_));
	materialMap_->color = material.color;
	materialMap_->enableLighting = material.enableLighting;
	materialMap_->uvTransform = MakeIdentity();
	textureIndex_ = material.textureIndex;
}
} // namespace KujakuEngine
