#include "Model.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "DirectionalLight.h"
#include "GraphicsPipeline.h"
#include "ModelUtil.h"
#include "PointLight.h"
#include "SpotLight.h"
#include <numbers>

namespace KujakuEngine {

Model* Model::CreateFromOBJ(const std::string& objname, ShaderModel shaderModel) {
	Model* model = new Model();
	std::string directoryPathFinal = "Resources/" + objname;
	std::string filename = objname + ".obj";

	ModelData rawData = ModelUtil::LoadModelFile(directoryPathFinal, filename);
	rawData.material.enableLighting = static_cast<int32_t>(shaderModel);
	ModelUtil::ResolveTextureIndex(rawData.material);
	model->rootLocalMatrix_ = rawData.rootNode.localMatrix;
	model->CreateVertexBuffer(rawData.vertices);
	model->CreateMaterialBuffer(rawData.material);
	return model;
}

Model* Model::CreateFromGlTF(const std::string& objname, ShaderModel shaderModel) {
	Model* model = new Model();
	std::string directoryPathFinal = "Resources/" + objname;
	std::string filename = objname + ".gltf";

	ModelData rawData = ModelUtil::LoadModelFile(directoryPathFinal, filename);
	rawData.material.enableLighting = static_cast<int32_t>(shaderModel);
	ModelUtil::ResolveTextureIndex(rawData.material);
	model->rootLocalMatrix_ = rawData.rootNode.localMatrix;
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

			vertices[startIndex].position = { cosf(lat) * cosf(lon), sinf(lat), cosf(lat) * sinf(lon), 1.0f };
			vertices[startIndex].texcoord = { u0, v0 };
			vertices[startIndex].normal = { vertices[startIndex].position.x, vertices[startIndex].position.y, vertices[startIndex].position.z };

			vertices[startIndex + 1].position = { cosf(lat + kLatEvery) * cosf(lon), sinf(lat + kLatEvery), cosf(lat + kLatEvery) * sinf(lon), 1.0f };
			vertices[startIndex + 1].texcoord = { u0, v1 };
			vertices[startIndex + 1].normal = { vertices[startIndex + 1].position.x, vertices[startIndex + 1].position.y, vertices[startIndex + 1].position.z };

			vertices[startIndex + 2].position = { cosf(lat) * cosf(lon + kLonEvery), sinf(lat), cosf(lat) * sinf(lon + kLonEvery), 1.0f };
			vertices[startIndex + 2].texcoord = { u1, v0 };
			vertices[startIndex + 2].normal = { vertices[startIndex + 2].position.x, vertices[startIndex + 2].position.y, vertices[startIndex + 2].position.z };

			vertices[startIndex + 3].position = { cosf(lat) * cosf(lon + kLonEvery), sinf(lat), cosf(lat) * sinf(lon + kLonEvery), 1.0f };
			vertices[startIndex + 3].texcoord = { u1, v0 };
			vertices[startIndex + 3].normal = { vertices[startIndex + 3].position.x, vertices[startIndex + 3].position.y, vertices[startIndex + 3].position.z };

			vertices[startIndex + 4].position = { cosf(lat + kLatEvery) * cosf(lon), sinf(lat + kLatEvery), cosf(lat + kLatEvery) * sinf(lon), 1.0f };
			vertices[startIndex + 4].texcoord = { u0, v1 };
			vertices[startIndex + 4].normal = { vertices[startIndex + 4].position.x, vertices[startIndex + 4].position.y, vertices[startIndex + 4].position.z };

			vertices[startIndex + 5].position = { cosf(lat + kLatEvery) * cosf(lon + kLonEvery), sinf(lat + kLatEvery), cosf(lat + kLatEvery) * sinf(lon + kLonEvery), 1.0f };
			vertices[startIndex + 5].texcoord = { u1, v1 };
			vertices[startIndex + 5].normal = { vertices[startIndex + 5].position.x, vertices[startIndex + 5].position.y, vertices[startIndex + 5].position.z };
		}
	}

	// MaterialData
	MaterialData defaultMaterial = ModelUtil::CreateTexturedMaterial(textureFilePath, static_cast<int32_t>(shaderModel));

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
	MaterialData defaultMaterial = ModelUtil::CreateTexturedMaterial(textureFilePath, static_cast<int32_t>(shaderModel));

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
	MaterialData defaultMaterial = ModelUtil::CreateTexturedMaterial(textureFilePath, static_cast<int32_t>(shaderModel));

	model->CreateVertexBuffer(vertices);
	model->CreateMaterialBuffer(defaultMaterial);

	return model;
}

Model* Model::CreateTriangle(const std::string& textureFilePath, ShaderModel shaderModel) {
	Model* model = new Model();

	std::vector<VertexData> vertices;

	const float halfWidth = std::numbers::sqrt3_v<float> *0.5f;

	vertices.push_back({
		.position = {0.0f, 1.0f, 0.0f, 1.0f},
		.texcoord = {0.5f, 0.0f},
		.normal = {0.0f, 0.0f, 1.0f}
		}); // 上

	vertices.push_back({
		.position = {halfWidth, -0.5f, 0.0f, 1.0f},
		.texcoord = {1.0f, 1.0f},
		.normal = {0.0f, 0.0f, 1.0f}
		}); // 右下

	vertices.push_back({
		.position = {-halfWidth, -0.5f, 0.0f, 1.0f},
		.texcoord = {0.0f, 1.0f},
		.normal = {0.0f, 0.0f, 1.0f}
		}); // 左下

	// MaterialData
	MaterialData defaultMaterial = ModelUtil::CreateTexturedMaterial(textureFilePath, static_cast<int32_t>(shaderModel));

	model->CreateVertexBuffer(vertices);
	model->CreateMaterialBuffer(defaultMaterial);

	return model;
}

Model* Model::CreateTetrahedron(const std::string& textureFilePath, ShaderModel shaderModel) {
	Model* model = new Model();

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
	MaterialData defaultMaterial = ModelUtil::CreateTexturedMaterial(textureFilePath, static_cast<int32_t>(shaderModel));

	model->CreateVertexBuffer(vertices);
	model->CreateMaterialBuffer(defaultMaterial);

	return model;
}
void Model::PreDraw() {
	ModelUtil::SetCommonRenderState();
}

void Model::PostDraw() {
	// 将来的にここで描画状態のリセットなどを行う
}

void Model::Draw(const WorldTransform& worldTransform, const Camera& camera, FillMode fillMode) {
	ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();

	// RootNodeのローカル行列をモデル固有の補正として描画時だけ適用する
	Matrix4x4 modelWorldMatrix = rootLocalMatrix_ * worldTransform.matWorld_;
	worldTransform.TransferMatrix(camera, modelWorldMatrix);

	// RootSignature と PSO をセット
	if (fillMode == kFillModeSolid) {
		GraphicsPipeline::GetInstance()->SetCommandList(PipelineType::kObject3d, blendMode_);
	}
	else if (fillMode == kFillModeWireframe) {
		GraphicsPipeline::GetInstance()->SetCommandList(PipelineType::kObject3dWireframe, blendMode_);
	}

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
	commandList->SetGraphicsRootConstantBufferView(6, SpotLight::GetInstance()->GetResource()->GetGPUVirtualAddress());

	// 描画
	commandList->DrawInstanced(vertexCount_, 1, 0, 0);
}

void Model::CreateVertexBuffer(const std::vector<VertexData>& vertices) {
	// 頂点保存
	vertices_ = vertices;

	// 頂点数を取得
	vertexCount_ = static_cast<uint32_t>(vertices.size());

	// 頂点数分のリソース作成
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

	materialResource_ = DirectXCommon::GetInstance()->CreateBufferResource((sizeof(MaterialData) + 0xff) & ~0xff);

	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialMap_));
	materialMap_->color = material.color;
	materialMap_->enableLighting = material.enableLighting;
	materialMap_->uvTransform = MakeIdentity();
	materialMap_->shininess = material.shininess;
	textureIndex_ = material.textureIndex;
}
} // namespace KujakuEngine
