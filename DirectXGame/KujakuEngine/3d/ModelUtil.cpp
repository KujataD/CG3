#include "ModelUtil.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace KujakuEngine::ModelUtil {
namespace {

std::string ResolveTexturePath(const std::string& directoryPath, const std::string& textureFilePath) {
	// OBJ/MTLから見た相対パスは、モデルのディレクトリ基準に変換する。
	std::filesystem::path texturePath(textureFilePath);
	if (texturePath.is_absolute()) {
		return texturePath.generic_string();
	}

	std::filesystem::path resolvedPath = std::filesystem::path(directoryPath) / texturePath;
	return resolvedPath.generic_string();
}

} // namespace

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;

	// Assimpを使ってOBJファイルを読む
	// ------------------------------------------
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	// aiProcess_FlipWindingOrder: 三角形の並び順を逆にする。
	// aiProcess_Triangulate: 四角形以上の面を三角形に分割する。
	// aiProcess_FlipUVs: UVをフリップする。
	const unsigned int importFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_FlipUVs;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), importFlags);
	assert(scene);
	assert(scene->HasMeshes()); // メッシュがないモデルは対応しない

	// meshを解析する
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());         // 法線がないMeshは今回は非対応
		assert(mesh->HasTextureCoords(0));  // TexcoordがないMeshは今回は非対応

		// faceを解析する
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // aiProcess_Triangulate後なので三角形のみ来る想定

			// vertexを解析する
			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

				VertexData vertex;
				vertex.position = {position.x, position.y, position.z, 1.0f};
				vertex.normal = {normal.x, normal.y, normal.z};
				vertex.texcoord = {texcoord.x, texcoord.y};

				// aiProcess_MakeLeftHandedはZ反転なので、既存のX反転に合わせて手動変換する。
				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;
				modelData.vertices.push_back(vertex);
			}
		}
	}

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
		aiMaterial* material = scene->mMaterials[materialIndex];

		// aiTextureType_DIFFUSE: テクスチャを模様として利用する
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.material.textureFilePath = ResolveTexturePath(directoryPath, textureFilePath.C_Str());
		}
	}

	return modelData;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	// MTLのmap_Kdからディフューズテクスチャを取得する。
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 絶対パスならそのまま、相対パスならモデルディレクトリ基準に変換する。
			materialData.textureFilePath = ResolveTexturePath(directoryPath, textureFilename);
		}
	}

	return materialData;
}

void ResolveTextureIndex(MaterialData& material) {
	TextureManager* textureManager = TextureManager::GetInstance();
	// テクスチャ指定がないモデルは、白1x1テクスチャを使って描画可能にする。
	if (!material.textureFilePath.empty()) {
		material.textureIndex = textureManager->LoadTexture(material.textureFilePath);
		return;
	}

	material.textureIndex = textureManager->GetDefaultWhiteTexture();
}

MaterialData CreateTexturedMaterial(const std::string& textureFilePath, int32_t enableLighting) {
	// プリミティブ生成用の基本マテリアルを作成する。
	MaterialData material;
	material.enableLighting = enableLighting;
	material.textureFilePath = textureFilePath;
	ResolveTextureIndex(material);
	return material;
}

void SetCommonRenderState() {
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

	// プリミティブトポロジの設定（main.cppで毎フレームセットしていたものに対応）
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

} // namespace KujakuEngine::ModelUtil
