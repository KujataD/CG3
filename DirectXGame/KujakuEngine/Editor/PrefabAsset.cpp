#include "PrefabAsset.h"
#include "../scene/Component.h"
#include "../scene/ComponentFactory.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 26819)
#endif
#include "../../externals/nlohmann/json.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace KujakuEngine {

namespace {

using json = nlohmann::json;

struct PrefabWriteObject {
	const GameObject* gameObject = nullptr;
	std::string prefabObjectId;
	std::string parentPrefabObjectId;
};

struct PendingParentLink {
	GameObject* gameObject = nullptr;
	std::string parentPrefabObjectId;
};

std::string EscapeJsonString(const std::string& text) {
	std::string escaped;
	escaped.reserve(text.size());

	for (char character : text) {
		if (character == '\\') {
			escaped += "\\\\";
		} else if (character == '"') {
			escaped += "\\\"";
		} else if (character == '\n') {
			escaped += "\\n";
		} else if (character == '\r') {
			escaped += "\\r";
		} else if (character == '\t') {
			escaped += "\\t";
		} else {
			escaped += character;
		}
	}

	return escaped;
}

std::string SanitizeFileName(const std::string& name) {
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

	if (sanitized.empty()) {
		return "Prefab";
	}

	return sanitized;
}

std::filesystem::path NormalizePath(const std::filesystem::path& path) {
	std::error_code error;
	std::filesystem::path normalized = std::filesystem::weakly_canonical(path, error);
	if (error) {
		return path.lexically_normal();
	}
	return normalized.lexically_normal();
}

std::filesystem::path MakeUniquePrefabPath(const std::filesystem::path& prefabDirectory, const std::string& objectName) {
	std::string baseName = SanitizeFileName(objectName);
	std::filesystem::path candidate = prefabDirectory / (baseName + ".prefab.json");
	if (!std::filesystem::exists(candidate)) {
		return candidate;
	}

	for (uint32_t index = 1; index < 10000; ++index) {
		std::ostringstream suffix;
		suffix << baseName << "_" << index << ".prefab.json";
		candidate = prefabDirectory / suffix.str();
		if (!std::filesystem::exists(candidate)) {
			return candidate;
		}
	}

	return prefabDirectory / (baseName + "_9999.prefab.json");
}

bool WriteTextFile(const std::filesystem::path& path, const std::string& text, std::string& message) {
	std::ofstream file(path, std::ios::binary);
	if (!file) {
		message = "Failed to open Prefab file: " + path.string();
		return false;
	}

	file << text;
	if (!file) {
		message = "Failed to write Prefab file: " + path.string();
		return false;
	}

	return true;
}

bool LoadJsonFile(const std::filesystem::path& path, json& outJson, std::string& message) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		message = "Failed to open Prefab file: " + path.string();
		return false;
	}

	try {
		file >> outJson;
	} catch (const json::exception& exception) {
		message = "Failed to parse Prefab JSON: " + path.string() + " : " + exception.what();
		return false;
	}

	return true;
}

std::string ReadString(const json& value, const std::string& key, const std::string& defaultValue) {
	if (!value.contains(key)) {
		return defaultValue;
	}
	if (!value.at(key).is_string()) {
		return defaultValue;
	}
	return value.at(key).get<std::string>();
}

bool ReadBool(const json& value, const std::string& key, bool defaultValue) {
	if (!value.contains(key)) {
		return defaultValue;
	}
	if (!value.at(key).is_boolean()) {
		return defaultValue;
	}
	return value.at(key).get<bool>();
}

bool IsTransformTypeName(const std::string& typeName) {
	if (typeName == "Transform") {
		return true;
	}
	if (typeName == "TransformComponent") {
		return true;
	}
	return false;
}

const json& GetComponentPropertiesJson(const json& componentJson) {
	if (componentJson.contains("properties") && componentJson.at("properties").is_object()) {
		return componentJson.at("properties");
	}

	// 旧形式や手書きJSONではComponent固有値が直下にある可能性も許容する。
	return componentJson;
}

void CollectPrefabWriteObjects(
    const GameObject& gameObject,
    const std::string& parentPrefabObjectId,
    std::vector<PrefabWriteObject>& outObjects) {
	std::ostringstream idStream;
	idStream << "object_" << outObjects.size();

	PrefabWriteObject writeObject{};
	writeObject.gameObject = &gameObject;
	writeObject.prefabObjectId = idStream.str();
	writeObject.parentPrefabObjectId = parentPrefabObjectId;
	outObjects.push_back(writeObject);

	const std::string currentPrefabObjectId = writeObject.prefabObjectId;
	for (GameObject* child : gameObject.GetChildren()) {
		if (child) {
			CollectPrefabWriteObjects(*child, currentPrefabObjectId, outObjects);
		}
	}
}

void WriteComponentsJson(std::ostream& os, const GameObject& gameObject, int indent) {
	const std::vector<std::unique_ptr<Component>>& components = gameObject.GetComponents();
	for (size_t componentIndex = 0; componentIndex < components.size(); ++componentIndex) {
		const std::unique_ptr<Component>& component = components[componentIndex];
		if (!component) {
			continue;
		}

		component->WriteJson(os, indent);
		if (componentIndex + 1 < components.size()) {
			os << ",";
		}
		os << "\n";
	}
}

std::string BuildPrefabJson(const GameObject& rootObject, size_t& outGameObjectCount, size_t& outComponentCount) {
	std::vector<PrefabWriteObject> writeObjects;
	CollectPrefabWriteObjects(rootObject, "", writeObjects);
	outGameObjectCount = writeObjects.size();

	std::ostringstream os;
	os << "{\n";
	os << "  \"assetType\": \"Prefab\",\n";
	os << "  \"version\": 1,\n";
	os << "  \"name\": \"" << EscapeJsonString(rootObject.GetName()) << "\",\n";
	os << "  \"rootObjectId\": \"object_0\",\n";
	os << "  \"objects\": [\n";

	for (size_t objectIndex = 0; objectIndex < writeObjects.size(); ++objectIndex) {
		const PrefabWriteObject& writeObject = writeObjects[objectIndex];
		const GameObject* gameObject = writeObject.gameObject;
		if (!gameObject) {
			continue;
		}

		os << "    {\n";
		os << "      \"prefabObjectId\": \"" << EscapeJsonString(writeObject.prefabObjectId) << "\",\n";
		os << "      \"parentPrefabObjectId\": \"" << EscapeJsonString(writeObject.parentPrefabObjectId) << "\",\n";
		os << "      \"name\": \"" << EscapeJsonString(gameObject->GetName()) << "\",\n";
		os << "      \"active\": ";
		if (gameObject->IsActive()) {
			os << "true,\n";
		} else {
			os << "false,\n";
		}
		os << "      \"components\": [\n";

		const std::vector<std::unique_ptr<Component>>& components = gameObject->GetComponents();
		outComponentCount += components.size();
		WriteComponentsJson(os, *gameObject, 8);

		os << "      ]\n";
		os << "    }";
		if (objectIndex + 1 < writeObjects.size()) {
			os << ",";
		}
		os << "\n";
	}

	os << "  ]\n";
	os << "}\n";
	return os.str();
}

void ApplyComponent(Component& component, const json& componentJson) {
	component.SetEnabled(ReadBool(componentJson, "enabled", component.IsEnabled()));
	component.ReadJson(GetComponentPropertiesJson(componentJson));
}

void NotifyComponentsAfterReadJson(GameObject& gameObject) {
	for (const std::unique_ptr<Component>& component : gameObject.GetComponents()) {
		if (component) {
			component->OnAfterReadJson();
		}
	}
}

size_t ApplyPrefabComponents(Scene& scene, GameObject& gameObject, const json& gameObjectJson) {
	size_t componentCount = 0;

	Component* transformComponent = gameObject.EnsureTransformComponent();
	if (transformComponent) {
		++componentCount;
	}

	if (!gameObjectJson.contains("components") || !gameObjectJson.at("components").is_array()) {
		NotifyComponentsAfterReadJson(gameObject);
		return componentCount;
	}

	for (const json& componentJson : gameObjectJson.at("components")) {
		if (!componentJson.is_object()) {
			continue;
		}

		std::string typeName = ReadString(componentJson, "type", "");
		if (typeName.empty()) {
			continue;
		}

		Component* component = nullptr;
		if (IsTransformTypeName(typeName)) {
			component = gameObject.EnsureTransformComponent();
		} else {
			std::unique_ptr<Component> createdComponent = ComponentFactory::GetInstance().Create(typeName);
			component = gameObject.AddComponent(std::move(createdComponent));
			if (component) {
				scene.OnEditorComponentAdded(&gameObject, component);
			}
		}

		if (!component) {
			continue;
		}

		ApplyComponent(*component, componentJson);
		++componentCount;
	}

	NotifyComponentsAfterReadJson(gameObject);
	return componentCount;
}

GameObject* FindInstantiatedObject(
    const std::unordered_map<std::string, GameObject*>& objectsByPrefabId,
    const std::string& prefabObjectId) {
	auto found = objectsByPrefabId.find(prefabObjectId);
	if (found == objectsByPrefabId.end()) {
		return nullptr;
	}
	return found->second;
}

} // namespace

PrefabAsset::SaveResult PrefabAsset::SaveAsPrefab(const GameObject& rootObject, const std::filesystem::path& projectRoot) {
	SaveResult result{};

	if (projectRoot.empty()) {
		result.message = "ProjectDir is empty.";
		return result;
	}

	std::filesystem::path normalizedProjectRoot = NormalizePath(projectRoot);
	std::filesystem::path prefabDirectory = normalizedProjectRoot / "Prefabs";

	std::error_code error;
	std::filesystem::create_directories(prefabDirectory, error);
	if (error) {
		result.message = "Failed to create Prefabs directory: " + error.message();
		return result;
	}

	result.outputPath = MakeUniquePrefabPath(prefabDirectory, rootObject.GetName());
	std::string prefabJson = BuildPrefabJson(rootObject, result.gameObjectCount, result.componentCount);
	if (!WriteTextFile(result.outputPath, prefabJson, result.message)) {
		return result;
	}

	result.succeeded = true;
	result.message = "Saved Prefab.";
	return result;
}

PrefabAsset::InstantiateResult PrefabAsset::Instantiate(Scene& scene, const std::filesystem::path& prefabPath) {
	InstantiateResult result{};

	json prefabJson;
	if (!LoadJsonFile(prefabPath, prefabJson, result.message)) {
		return result;
	}

	std::string assetType = ReadString(prefabJson, "assetType", "");
	if (!assetType.empty() && assetType != "Prefab") {
		result.message = "JSON is not a Prefab: " + prefabPath.string();
		return result;
	}

	std::vector<json> objectEntries;
	if (prefabJson.contains("objects") && prefabJson.at("objects").is_array()) {
		for (const json& objectEntry : prefabJson.at("objects")) {
			if (objectEntry.is_object()) {
				objectEntries.push_back(objectEntry);
			}
		}
	} else {
		objectEntries.push_back(prefabJson);
	}

	if (objectEntries.empty()) {
		result.message = "Prefab does not contain GameObject entries: " + prefabPath.string();
		return result;
	}

	std::unordered_map<std::string, GameObject*> objectsByPrefabId;
	std::vector<PendingParentLink> parentLinks;
	std::string rootObjectId = ReadString(prefabJson, "rootObjectId", "object_0");

	for (size_t objectIndex = 0; objectIndex < objectEntries.size(); ++objectIndex) {
		const json& objectJson = objectEntries[objectIndex];

		std::string defaultPrefabObjectId = "object_" + std::to_string(objectIndex);
		std::string prefabObjectId = ReadString(objectJson, "prefabObjectId", defaultPrefabObjectId);
		std::string parentPrefabObjectId = ReadString(objectJson, "parentPrefabObjectId", "");
		std::string objectName = ReadString(objectJson, "name", "PrefabObject");

		GameObject* gameObject = scene.CreateGameObject(objectName);
		if (!gameObject) {
			continue;
		}

		gameObject->SetActive(ReadBool(objectJson, "active", gameObject->IsActive()));
		result.componentCount += ApplyPrefabComponents(scene, *gameObject, objectJson);

		objectsByPrefabId[prefabObjectId] = gameObject;
		parentLinks.push_back({gameObject, parentPrefabObjectId});
		++result.gameObjectCount;

		if (prefabObjectId == rootObjectId || !result.rootObject) {
			result.rootObject = gameObject;
		}
	}

	for (const PendingParentLink& link : parentLinks) {
		if (!link.gameObject) {
			continue;
		}

		GameObject* parent = FindInstantiatedObject(objectsByPrefabId, link.parentPrefabObjectId);
		link.gameObject->SetParent(parent);
	}

	result.succeeded = result.rootObject != nullptr;
	if (result.succeeded) {
		result.message = "Instantiated Prefab.";
	} else {
		result.message = "Failed to instantiate Prefab: " + prefabPath.string();
	}

	return result;
}

} // namespace KujakuEngine
