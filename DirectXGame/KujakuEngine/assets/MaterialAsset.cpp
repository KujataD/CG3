#include "MaterialAsset.h"
#include "../Editor/AssetDatabase.h"
#include "../Editor/EditorProjectPath.h"
#include "../base/TextureManager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>

namespace KujakuEngine {

namespace {

std::string ToLower(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return text;
}

std::string ReadString(const nlohmann::json& value, const char* key, const std::string& defaultValue) {
	if (!value.contains(key)) {
		return defaultValue;
	}
	if (!value.at(key).is_string()) {
		return defaultValue;
	}
	return value.at(key).get<std::string>();
}

Vector4 ReadColor(const nlohmann::json& value, const Vector4& defaultColor) {
	if (!value.contains("color")) {
		return defaultColor;
	}
	if (!value.at("color").is_array()) {
		return defaultColor;
	}
	if (value.at("color").size() < 4) {
		return defaultColor;
	}

	Vector4 color = defaultColor;
	for (size_t index = 0; index < 4; ++index) {
		if (value.at("color").at(index).is_number()) {
			(&color.x)[index] = value.at("color").at(index).get<float>();
		}
	}
	return color;
}

std::filesystem::path ResolveProjectPath(const std::filesystem::path& path) {
	if (path.empty()) {
		return {};
	}
	if (path.is_absolute()) {
		return NormalizeEditorPath(path);
	}
	return NormalizeEditorPath(DetectEditorProjectRoot() / path);
}

std::string GetMaterialNameFromPath(const std::filesystem::path& path) {
	std::string fileName = path.filename().string();
	const std::string suffix = ".material.json";
	if (fileName.ends_with(suffix)) {
		fileName.erase(fileName.size() - suffix.size());
	}
	if (fileName.empty()) {
		return "New Material";
	}
	return fileName;
}

} // namespace

MaterialAssetData MaterialAsset::CreateDefault() {
	MaterialAssetData material{};
	return material;
}

bool MaterialAsset::IsMaterialFile(const std::filesystem::path& path) {
	std::string fileName = ToLower(path.filename().string());
	return fileName.ends_with(".material.json");
}

bool MaterialAsset::Load(const std::filesystem::path& path, MaterialAssetData& outMaterial, std::string& message) {
	std::filesystem::path resolvedPath = ResolveProjectPath(path);
	std::ifstream file(resolvedPath, std::ios::binary);
	if (!file) {
		message = "Failed to open Material: " + resolvedPath.string();
		return false;
	}

	nlohmann::json materialJson;
	try {
		file >> materialJson;
	} catch (const nlohmann::json::exception& exception) {
		message = "Failed to parse Material: " + resolvedPath.string() + " : " + exception.what();
		return false;
	}

	outMaterial = ReadJsonObject(materialJson, CreateDefault());
	if (outMaterial.name.empty()) {
		outMaterial.name = GetMaterialNameFromPath(resolvedPath);
	}
	message = "Loaded Material.";
	return true;
}

bool MaterialAsset::Save(const std::filesystem::path& path, const MaterialAssetData& material, std::string& message) {
	std::filesystem::path resolvedPath = ResolveProjectPath(path);

	std::error_code error;
	std::filesystem::create_directories(resolvedPath.parent_path(), error);
	if (error) {
		message = "Failed to create Material directory: " + error.message();
		return false;
	}

	nlohmann::json materialJson;
	materialJson["assetType"] = "Material";
	materialJson["version"] = 1;
	WriteJsonObject(materialJson, material);

	std::ofstream file(resolvedPath, std::ios::binary);
	if (!file) {
		message = "Failed to open Material: " + resolvedPath.string();
		return false;
	}

	file << materialJson.dump(2);
	file << "\n";
	if (!file) {
		message = "Failed to write Material: " + resolvedPath.string();
		return false;
	}

	AssetDatabase::GetInstance().GetOrCreateAssetId(resolvedPath);
	message = "Saved Material.";
	return true;
}

bool MaterialAsset::CreateDefaultFile(const std::filesystem::path& path, std::string& message) {
	MaterialAssetData material = CreateDefault();
	material.name = GetMaterialNameFromPath(path);
	return Save(path, material, message);
}

MaterialAssetData MaterialAsset::ReadJsonObject(const nlohmann::json& json, const MaterialAssetData& defaultMaterial) {
	MaterialAssetData material = defaultMaterial;
	if (!json.is_object()) {
		return material;
	}

	material.name = ReadString(json, "name", material.name);
	material.color = ReadColor(json, material.color);
	material.textureAssetId = ReadString(json, "textureAssetId", material.textureAssetId);
	material.texturePath = ReadString(json, "texturePath", material.texturePath);

	// 初期実装や旧Scene JSONではtextureという名前で保存していたため、互換用に読み取る。
	material.texturePath = ReadString(json, "texture", material.texturePath);
	return material;
}

void MaterialAsset::WriteJsonObject(nlohmann::json& json, const MaterialAssetData& material) {
	json["name"] = material.name;
	json["color"] = {material.color.x, material.color.y, material.color.z, material.color.w};
	json["textureAssetId"] = material.textureAssetId;
	json["texturePath"] = material.texturePath;
}

std::filesystem::path MaterialAsset::ResolveTexturePath(const MaterialAssetData& material) {
	AssetDatabase& assetDatabase = AssetDatabase::GetInstance();
	std::filesystem::path fallbackPath;
	if (!material.texturePath.empty()) {
		fallbackPath = material.texturePath;
	}

	std::filesystem::path resolvedPath = assetDatabase.ResolveAssetPath(material.textureAssetId, fallbackPath);
	if (!resolvedPath.empty()) {
		return resolvedPath;
	}

	return ResolveProjectPath("resources/white1x1.png");
}

uint32_t MaterialAsset::ResolveTextureIndex(const MaterialAssetData& material) {
	std::filesystem::path texturePath = ResolveTexturePath(material);
	uint32_t textureIndex = 0;
	if (!texturePath.empty()) {
		if (TextureManager::GetInstance()->TryLoadTexture(texturePath.string(), textureIndex)) {
			return textureIndex;
		}
	}

	return TextureManager::GetInstance()->GetDefaultWhiteTexture();
}

} // namespace KujakuEngine
