#include "Model.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include "DirectionalLight.h"
#include "GraphicsPipeline.h"
#include "PointLight.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
namespace KujakuEngine {

Model* Model::CreateFromOBJ(const std::string& objname, ShaderModel shaderModel) {
	Model* model = new Model();
	std::string directoryPathFinal = "resources/" + objname;
	std::string filename = objname + ".obj";

	ModelData rawData = LoadObjFile(directoryPathFinal, filename);
	rawData.material.enableLighting = static_cast<int32_t>(shaderModel);
	if (!rawData.material.textureFilePath.empty()) {
		rawData.material.textureIndex = TextureManager::GetInstance()->LoadTexture(rawData.material.textureFilePath);
	} else {
		rawData.material.textureIndex = TextureManager::GetInstance()->GetDefaultWhiteTexture();
	}
	model->CreateVertexBuffer(rawData.vertices);
	model->CreateMaterialBuffer(rawData.material);
	return model;
}
Model* Model::CreateSphere(const std::string& textureFilePath, ShaderModel shaderModel, uint32_t subdivision) {
	Model* model = new Model();
	const float kLonEvery = static_cast<float>(2.0f * std::numbers::pi_v<float> / subdivision);
	const float kLatEvery = static_cast<float>(std::numbers::pi_v<float> / subdivision);

	std::vector<VertexData> vertices;
	vertices.resize(6 * subdivision * subdivision);

	for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
		float lat = static_cast<float>(-std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex);

		for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery;

			float u0 = float(lonIndex) / float(subdivision);
			float u1 = float(lonIndex + 1) / float(subdivision);

			float v0 = 1.0f - float(latIndex) / float(subdivision);
			float v1 = 1.0f - float(latIndex + 1) / float(subdivision);

			uint32_t startIndex = (latIndex * subdivision + lonIndex) * 6;

			vertices[startIndex].position = {cosf(lat) * cosf(lon), sinf(lat), cosf(lat) * sinf(lon), 1.0f};
			vertices[startIndex].texcoord = {u0, v0};
			vertices[startIndex].normal = {vertices[startIndex].position.x, vertices[startIndex].position.y, vertices[startIndex].position.z};

			vertices[startIndex + 1].position = {cosf(lat + kLatEvery) * cosf(lon), sinf(lat + kLatEvery), cosf(lat + kLatEvery) * sinf(lon), 1.0f};
			vertices[startIndex + 1].texcoord = {u0, v1};
			vertices[startIndex + 1].normal = {vertices[startIndex + 1].position.x, vertices[startIndex + 1].position.y, vertices[startIndex + 1].position.z};

			vertices[startIndex + 2].position = {cosf(lat) * cosf(lon + kLonEvery), sinf(lat), cosf(lat) * sinf(lon + kLonEvery), 1.0f};
			vertices[startIndex + 2].texcoord = {u1, v0};
			vertices[startIndex + 2].normal = {vertices[startIndex + 2].position.x, vertices[startIndex + 2].position.y, vertices[startIndex + 2].position.z};

			vertices[startIndex + 3].position = {cosf(lat) * cosf(lon + kLonEvery), sinf(lat), cosf(lat) * sinf(lon + kLonEvery), 1.0f};
			vertices[startIndex + 3].texcoord = {u1, v0};
			vertices[startIndex + 3].normal = {vertices[startIndex + 3].position.x, vertices[startIndex + 3].position.y, vertices[startIndex + 3].position.z};

			vertices[startIndex + 4].position = {cosf(lat + kLatEvery) * cosf(lon), sinf(lat + kLatEvery), cosf(lat + kLatEvery) * sinf(lon), 1.0f};
			vertices[startIndex + 4].texcoord = {u0, v1};
			vertices[startIndex + 4].normal = {vertices[startIndex + 4].position.x, vertices[startIndex + 4].position.y, vertices[startIndex + 4].position.z};

			vertices[startIndex + 5].position = {cosf(lat + kLatEvery) * cosf(lon + kLonEvery), sinf(lat + kLatEvery), cosf(lat + kLatEvery) * sinf(lon + kLonEvery), 1.0f};
			vertices[startIndex + 5].texcoord = {u1, v1};
			vertices[startIndex + 5].normal = {vertices[startIndex + 5].position.x, vertices[startIndex + 5].position.y, vertices[startIndex + 5].position.z};
		}
	}

	// MaterialData
	MaterialData defaultMaterial{};
	defaultMaterial.enableLighting = static_cast<int32_t>(shaderModel);
	defaultMaterial.textureIndex = TextureManager::GetInstance()->LoadTexture(textureFilePath);

	model->CreateVertexBuffer(vertices);
	model->CreateMaterialBuffer(defaultMaterial);

	return model;
}

Model* Model::CreateCube(const std::string& textureFilePath, ShaderModel shaderModel) {
	Model* model = new Model();

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
	defaultMaterial.enableLighting = static_cast<int32_t>(shaderModel);
	defaultMaterial.textureIndex = TextureManager::GetInstance()->LoadTexture(textureFilePath);

	model->CreateVertexBuffer(vertices);
	model->CreateMaterialBuffer(defaultMaterial);

	return model;
}

Model* Model::CreatePlane(const std::string& textureFilePath, ShaderModel shaderModel) {
	Model* model = new Model();

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
	defaultMaterial.enableLighting = static_cast<int32_t>(shaderModel);
	defaultMaterial.textureIndex = TextureManager::GetInstance()->LoadTexture(textureFilePath);

	model->CreateVertexBuffer(vertices);
	model->CreateMaterialBuffer(defaultMaterial);

	return model;
}

void Model::PreDraw() {
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

void Model::PostDraw() {
	// 将来的にここで描画状態のリセットなどを行う
}

void Model::Draw(const WorldTransform& worldTransform, const Camera& camera) {
	ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();

	// RootSignature と PSO をセット
	GraphicsPipeline::GetInstance()->SetCommandList(PipelineType::kObject3d, blendMode_);

	// VBVを設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// マテリアルCBuffer（RootParameter[0]: PixelShader, b0）
	commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

	// WVP・WorldCBuffer（RootParameter[1]: VertexShader, b0）
	commandList->SetGraphicsRootConstantBufferView(1, worldTransform.GetConstBuffer()->GetGPUVirtualAddress());

	// テクスチャSRV（RootParameter[2]: DescriptorTable）
	auto handle = TextureManager::GetInstance()->GetSrvHandle(textureIndex_);
	commandList->SetGraphicsRootDescriptorTable(2, handle);

	// directional light
	commandList->SetGraphicsRootConstantBufferView(3, DirectionalLight::GetInstance()->GetResource()->GetGPUVirtualAddress());

	// カメラ（RootParameter[4]: VertexShader, b2）
	commandList->SetGraphicsRootConstantBufferView(4, camera.GetCameraForGPUResource()->GetGPUVirtualAddress());

	// pointlight
	commandList->SetGraphicsRootConstantBufferView(5, PointLight::GetInstance()->GetResource()->GetGPUVirtualAddress());

	// 描画
	commandList->DrawInstanced(vertexCount_, 1, 0, 0);
}

MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
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

ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
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
				modelData.vertices.push_back(faceVertices[0]);
				modelData.vertices.push_back(faceVertices[i + 1]);
				modelData.vertices.push_back(faceVertices[i]);
			}
		} else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;
}

void Model::CreateVertexBuffer(const std::vector<VertexData>& vertices) {
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

void Model::CreateMaterialBuffer(const MaterialData& material) {

	materialResource_ = DirectXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * vertexCount_);

	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialMap_));
	materialMap_->color = material.color;
	materialMap_->enableLighting = material.enableLighting;
	materialMap_->uvTransform = Matrix4x4::MakeIdentity();
	materialMap_->shininess = material.shininess;
	textureIndex_ = material.textureIndex;
}
} // namespace KujakuEngine
