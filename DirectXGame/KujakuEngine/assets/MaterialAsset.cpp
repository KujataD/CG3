#include "MaterialAsset.h"
#include "../runtime/AssetResolver.h"
#include "../base/ProjectPath.h"
#include "../base/TextureManager.h"

#include <algorithm>
#include <array>
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

std::filesystem::path ResolveProjectPath(const std::filesystem::path& path);

Vector4 ReadVector4(const nlohmann::json& value, const char* key, const Vector4& defaultValue) {
	if (!value.contains(key)) {
		return defaultValue;
	}
	if (!value.at(key).is_array()) {
		return defaultValue;
	}
	if (value.at(key).size() < 4) {
		return defaultValue;
	}

	Vector4 result = defaultValue;
	for (size_t index = 0; index < 4; ++index) {
		if (value.at(key).at(index).is_number()) {
			(&result.x)[index] = value.at(key).at(index).get<float>();
		}
	}
	return result;
}

std::array<MaterialTextureSlot, 3> GetKnownTextureSlots() {
	return {
	    MaterialTextureSlot::BaseColor,
	    MaterialTextureSlot::Normal,
	    MaterialTextureSlot::Environment,
	};
}

MaterialTexture* FindTexture(MaterialAssetData& material, MaterialTextureSlot slot) {
	for (MaterialTexture& texture : material.textures) {
		if (texture.slot == slot) {
			return &texture;
		}
	}
	return nullptr;
}

bool ReadTextureSlotObject(const nlohmann::json& textureJson, MaterialTextureSlot slot, MaterialAssetData& material) {
	if (!textureJson.is_object() && !textureJson.is_string()) {
		return false;
	}

	std::string assetId;
	std::string path;
	if (textureJson.is_string()) {
		path = textureJson.get<std::string>();
	} else {
		assetId = ReadString(textureJson, "assetId", assetId);
		path = ReadString(textureJson, "path", path);
	}

	MaterialAsset::SetTexture(material, slot, assetId, path);
	return true;
}

bool ReadTexturesObject(const nlohmann::json& json, MaterialAssetData& material) {
	if (!json.contains("textures")) {
		return false;
	}

	const nlohmann::json& texturesJson = json.at("textures");
	bool readAnyTexture = false;

	if (texturesJson.is_object()) {
		for (auto iterator = texturesJson.begin(); iterator != texturesJson.end(); ++iterator) {
			MaterialTextureSlot slot = MaterialTextureSlot::BaseColor;
			if (!MaterialAsset::TryGetTextureSlotFromString(iterator.key(), slot)) {
				continue;
			}
			if (ReadTextureSlotObject(iterator.value(), slot, material)) {
				readAnyTexture = true;
			}
		}
		return readAnyTexture;
	}

	if (texturesJson.is_array()) {
		for (const nlohmann::json& textureJson : texturesJson) {
			if (!textureJson.is_object()) {
				continue;
			}

			MaterialTextureSlot slot = MaterialTextureSlot::BaseColor;
			std::string slotName = ReadString(textureJson, "slot", "");
			if (!MaterialAsset::TryGetTextureSlotFromString(slotName, slot)) {
				continue;
			}
			if (ReadTextureSlotObject(textureJson, slot, material)) {
				readAnyTexture = true;
			}
		}
	}

	return readAnyTexture;
}

std::filesystem::path ResolveMaterialTexturePath(const MaterialTexture* texture, MaterialTextureSlot slot) {
	if (!texture) {
		if (slot == MaterialTextureSlot::BaseColor) {
			return ResolveProjectPath("resources/white1x1.png");
		}
		return {};
	}

	IAssetResolver& assetDatabase = GetAssetResolver();
	std::filesystem::path fallbackPath;
	if (!texture->path.empty()) {
		fallbackPath = texture->path;
	}

	std::filesystem::path resolvedPath = assetDatabase.ResolveAssetPath(texture->assetId, fallbackPath);
	if (!resolvedPath.empty()) {
		return resolvedPath;
	}

	if (slot == MaterialTextureSlot::BaseColor) {
		return ResolveProjectPath("resources/white1x1.png");
	}

	return {};
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

std::filesystem::path GetMetaPath(const std::filesystem::path& assetPath) {
	std::filesystem::path metaPath = assetPath;
	metaPath += ".meta";
	return metaPath;
}

} // namespace

MaterialAssetData MaterialAsset::CreateDefault() {
	MaterialAssetData material{};
	SetTexture(material, MaterialTextureSlot::BaseColor, "", "resources/white1x1.png");
	return material;
}

bool MaterialAsset::IsMaterialFile(const std::filesystem::path& path) {
	std::string fileName = ToLower(path.filename().string());
	return fileName.ends_with(".material.json");
}

const char* MaterialAsset::ToString(MaterialTextureSlot slot) {
	switch (slot) {
	case MaterialTextureSlot::BaseColor:
		return "BaseColor";
	case MaterialTextureSlot::Normal:
		return "Normal";
	case MaterialTextureSlot::Environment:
		return "Environment";
	default:
		return "BaseColor";
	}
}

bool MaterialAsset::TryGetTextureSlotFromString(const std::string& slotName, MaterialTextureSlot& outSlot) {
	std::string lowerName = ToLower(slotName);
	if (lowerName == "basecolor" || lowerName == "base_color" || lowerName == "albedo" || lowerName == "diffuse") {
		outSlot = MaterialTextureSlot::BaseColor;
		return true;
	}
	if (lowerName == "normal" || lowerName == "normalmap" || lowerName == "normal_map") {
		outSlot = MaterialTextureSlot::Normal;
		return true;
	}
	if (lowerName == "environment" || lowerName == "environmentmap" || lowerName == "environment_map" || lowerName == "env") {
		outSlot = MaterialTextureSlot::Environment;
		return true;
	}

	return false;
}

const MaterialTexture* MaterialAsset::GetTexture(const MaterialAssetData& material, MaterialTextureSlot slot) {
	for (const MaterialTexture& texture : material.textures) {
		if (texture.slot == slot) {
			return &texture;
		}
	}
	return nullptr;
}

std::string MaterialAsset::GetTexturePath(const MaterialAssetData& material, MaterialTextureSlot slot) {
	const MaterialTexture* texture = GetTexture(material, slot);
	if (!texture) {
		return "";
	}
	return texture->path;
}

void MaterialAsset::SetTexture(MaterialAssetData& material, MaterialTextureSlot slot, const std::string& assetId, const std::string& path) {
	MaterialTexture* texture = FindTexture(material, slot);
	if (!texture) {
		MaterialTexture newTexture{};
		newTexture.slot = slot;
		material.textures.push_back(newTexture);
		texture = &material.textures.back();
	}

	texture->assetId = assetId;
	texture->path = path;
}

std::string MaterialAsset::GetDisplayName(const std::filesystem::path& path) {
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

std::string MaterialAsset::SanitizeName(const std::string& name) {
	std::string sanitized;
	sanitized.reserve(name.size());

	for (char character : name) {
		bool invalid = character == '<' || character == '>' || character == ':' || character == '"' || character == '/' ||
		               character == '\\' || character == '|' || character == '?' || character == '*';
		if (invalid) {
			sanitized += '_';
		} else {
			sanitized += character;
		}
	}

	while (!sanitized.empty() && sanitized.front() == ' ') {
		sanitized.erase(sanitized.begin());
	}
	while (!sanitized.empty() && sanitized.back() == ' ') {
		sanitized.pop_back();
	}

	if (sanitized.empty()) {
		return "New Material";
	}
	return sanitized;
}

bool MaterialAsset::Rename(const std::filesystem::path& path, const std::string& newName, std::filesystem::path& outNewPath, std::string& message) {
	std::filesystem::path oldPath = ResolveProjectPath(path);
	if (!std::filesystem::exists(oldPath)) {
		message = "Material file was not found.";
		return false;
	}
	if (!IsMaterialFile(oldPath)) {
		message = "Selected file is not Material.";
		return false;
	}

	MaterialAssetData material{};
	if (!Load(oldPath, material, message)) {
		return false;
	}

	std::string sanitizedName = SanitizeName(newName);
	std::filesystem::path newPath = oldPath.parent_path() / (sanitizedName + ".material.json");
	newPath = ResolveProjectPath(newPath);

	std::error_code error;
	if (newPath != oldPath && std::filesystem::exists(newPath, error)) {
		message = "Same name Material already exists.";
		return false;
	}

	std::filesystem::path oldMetaPath = GetMetaPath(oldPath);
	std::filesystem::path newMetaPath = GetMetaPath(newPath);
	if (newMetaPath != oldMetaPath && std::filesystem::exists(newMetaPath, error)) {
		message = "Same name meta already exists.";
		return false;
	}

	if (newPath != oldPath) {
		std::filesystem::rename(oldPath, newPath, error);
		if (error) {
			message = "Failed to rename Material: " + error.message();
			return false;
		}

		if (std::filesystem::exists(oldMetaPath, error)) {
			std::filesystem::rename(oldMetaPath, newMetaPath, error);
			if (error) {
				message = "Failed to rename Material meta: " + error.message();
				return false;
			}
		}
	}

	material.name = sanitizedName;
	if (!Save(newPath, material, message)) {
		return false;
	}

	outNewPath = newPath;
	message = "Renamed Material.";
	return true;
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
		outMaterial.name = GetDisplayName(resolvedPath);
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

	GetAssetResolver().GetOrCreateAssetId(resolvedPath);
	message = "Saved Material.";
	return true;
}

bool MaterialAsset::CreateDefaultFile(const std::filesystem::path& path, std::string& message) {
	MaterialAssetData material = CreateDefault();
	material.name = GetDisplayName(path);
	return Save(path, material, message);
}

MaterialAssetData MaterialAsset::ReadJsonObject(const nlohmann::json& json, const MaterialAssetData& defaultMaterial) {
	MaterialAssetData material = defaultMaterial;
	if (!json.is_object()) {
		return material;
	}

	material.name = ReadString(json, "name", material.name);
	material.baseColor = ReadVector4(json, "baseColor", material.baseColor);
	// 旧Material JSONではcolorという名前だったため、互換用に読み取る。
	material.baseColor = ReadVector4(json, "color", material.baseColor);

	bool readNewTextures = ReadTexturesObject(json, material);
	if (!readNewTextures) {
		std::string legacyAssetId = ReadString(json, "textureAssetId", "");
		std::string legacyTexturePath = ReadString(json, "texturePath", "");
		// 初期実装や旧Scene JSONではtextureという名前で保存していたため、互換用に読み取る。
		legacyTexturePath = ReadString(json, "texture", legacyTexturePath);
		if (!legacyAssetId.empty() || !legacyTexturePath.empty()) {
			SetTexture(material, MaterialTextureSlot::BaseColor, legacyAssetId, legacyTexturePath);
		}
	}

	return material;
}

void MaterialAsset::WriteJsonObject(nlohmann::json& json, const MaterialAssetData& material) {
	json["name"] = material.name;
	json["baseColor"] = {material.baseColor.x, material.baseColor.y, material.baseColor.z, material.baseColor.w};

	nlohmann::json texturesJson = nlohmann::json::object();
	for (MaterialTextureSlot slot : GetKnownTextureSlots()) {
		const MaterialTexture* texture = GetTexture(material, slot);
		nlohmann::json textureJson;
		if (texture) {
			textureJson["assetId"] = texture->assetId;
			textureJson["path"] = texture->path;
		} else {
			textureJson["assetId"] = "";
			textureJson["path"] = "";
		}
		texturesJson[ToString(slot)] = textureJson;
	}
	json["textures"] = texturesJson;
}

std::filesystem::path MaterialAsset::ResolveTexturePath(const MaterialAssetData& material, MaterialTextureSlot slot) {
	return ResolveMaterialTexturePath(GetTexture(material, slot), slot);
}

uint32_t MaterialAsset::ResolveTextureIndex(const MaterialAssetData& material, MaterialTextureSlot slot) {
	std::filesystem::path texturePath = ResolveTexturePath(material, slot);
	uint32_t textureIndex = 0;
	if (!texturePath.empty()) {
		if (TextureManager::GetInstance()->TryLoadTexture(texturePath.string(), textureIndex)) {
			return textureIndex;
		}
	}

	if (slot != MaterialTextureSlot::BaseColor) {
		return 0;
	}

	return TextureManager::GetInstance()->GetDefaultWhiteTexture();
}

} // namespace KujakuEngine
