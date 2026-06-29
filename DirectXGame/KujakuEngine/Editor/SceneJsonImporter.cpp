#include "SceneJsonImporter.h"
#include "../components/ModelRendererComponent.h"
#include "../components/RotatorComponent.h"
#include "../components/TransformComponent.h"
#include "../components/TransformSnapshotComponent.h"
#include "../math/Vector3.h"
#include "../scene/ComponentFactory.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"
#include "../../externals/nlohmann/json.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace KujakuEngine {

namespace {

using json = nlohmann::json;

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
		return "GameObject";
	}

	return sanitized;
}

bool LoadJsonFile(const std::filesystem::path& path, json& outJson, std::string& message) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		message = "Failed to open JSON: " + path.string();
		return false;
	}

	try {
		file >> outJson;
	} catch (const json::exception& exception) {
		message = "Failed to parse JSON: " + path.string() + " : " + exception.what();
		return false;
	}

	return true;
}

std::filesystem::path ResolveAssetPath(const std::filesystem::path& projectRoot, const std::string& assetPathText) {
	std::filesystem::path assetPath(assetPathText);
	if (assetPath.is_absolute()) {
		return assetPath.lexically_normal();
	}
	return (projectRoot / assetPath).lexically_normal();
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

float ReadFloat(const json& value, const std::string& key, float defaultValue) {
	if (!value.contains(key)) {
		return defaultValue;
	}
	if (!value.at(key).is_number()) {
		return defaultValue;
	}
	return value.at(key).get<float>();
}

Vector3 ReadVector3(const json& value, const std::string& key, const Vector3& defaultValue) {
	if (!value.contains(key)) {
		return defaultValue;
	}

	const json& arrayValue = value.at(key);
	if (!arrayValue.is_array() || arrayValue.size() < 3) {
		return defaultValue;
	}
	if (!arrayValue[0].is_number() || !arrayValue[1].is_number() || !arrayValue[2].is_number()) {
		return defaultValue;
	}

	return {
	    arrayValue[0].get<float>(),
	    arrayValue[1].get<float>(),
	    arrayValue[2].get<float>(),
	};
}

bool ContainsPointer(const std::vector<GameObject*>& objects, GameObject* target) {
	return std::find(objects.begin(), objects.end(), target) != objects.end();
}

bool ContainsPointer(const std::vector<Component*>& components, Component* target) {
	return std::find(components.begin(), components.end(), target) != components.end();
}

GameObject* FindExistingGameObject(Scene& scene, size_t objectIndex, const std::string& objectName, const std::vector<GameObject*>& importedObjects) {
	std::vector<std::unique_ptr<GameObject>>& gameObjects = scene.GetGameObjects();

	if (objectIndex < gameObjects.size()) {
		GameObject* indexedObject = gameObjects[objectIndex].get();
		if (indexedObject && !ContainsPointer(importedObjects, indexedObject)) {
			return indexedObject;
		}
	}

	for (const std::unique_ptr<GameObject>& gameObject : gameObjects) {
		if (!gameObject) {
			continue;
		}
		if (ContainsPointer(importedObjects, gameObject.get())) {
			continue;
		}
		if (gameObject->GetName() == objectName) {
			return gameObject.get();
		}
	}

	return nullptr;
}

Component* FindExistingComponent(GameObject& gameObject, const std::string& typeName, const std::vector<Component*>& importedComponents) {
	for (const std::unique_ptr<Component>& component : gameObject.GetComponents()) {
		if (!component) {
			continue;
		}
		if (ContainsPointer(importedComponents, component.get())) {
			continue;
		}
		if (component->GetTypeName() == typeName) {
			return component.get();
		}
	}

	return nullptr;
}

ModelRendererComponent::PrimitiveType ReadPrimitiveType(const std::string& primitiveName) {
	if (primitiveName == "Cube") {
		return ModelRendererComponent::PrimitiveType::Cube;
	}

	if (primitiveName == "Sphere") {
		return ModelRendererComponent::PrimitiveType::Sphere;
	}

	if (primitiveName == "None") {
		return ModelRendererComponent::PrimitiveType::None;
	}

	return ModelRendererComponent::PrimitiveType::Custom;
}

void ApplyTransformComponent(TransformComponent& component, const json& componentJson) {
	WorldTransform& transform = component.GetTransform();
	transform.translation_ = ReadVector3(componentJson, "translation", transform.translation_);
	transform.rotation_ = ReadVector3(componentJson, "rotation", transform.rotation_);
	transform.scale_ = ReadVector3(componentJson, "scale", transform.scale_);
}

void ApplyRotatorComponent(RotatorComponent& component, const json& componentJson) {
	component.SetSpeed(ReadFloat(componentJson, "speed", component.GetSpeed()));
}

void ApplyModelRendererComponent(ModelRendererComponent& component, const json& componentJson) {
	std::string primitiveName = ReadString(componentJson, "primitive", "Custom");
	std::string textureFilePath = ReadString(componentJson, "texture", component.GetTextureFilePath());
	component.SetPrimitive(ReadPrimitiveType(primitiveName), textureFilePath);
}

void ApplyComponent(Component& component, const json& componentJson) {
	component.SetEnabled(ReadBool(componentJson, "enabled", component.IsEnabled()));

	TransformComponent* transformComponent = dynamic_cast<TransformComponent*>(&component);
	if (transformComponent) {
		ApplyTransformComponent(*transformComponent, componentJson);
		return;
	}

	RotatorComponent* rotatorComponent = dynamic_cast<RotatorComponent*>(&component);
	if (rotatorComponent) {
		ApplyRotatorComponent(*rotatorComponent, componentJson);
		return;
	}

	ModelRendererComponent* modelRendererComponent = dynamic_cast<ModelRendererComponent*>(&component);
	if (modelRendererComponent) {
		ApplyModelRendererComponent(*modelRendererComponent, componentJson);
		return;
	}
}

void RefreshTransformSnapshots(GameObject& gameObject) {
	for (const std::unique_ptr<Component>& component : gameObject.GetComponents()) {
		if (!component) {
			continue;
		}

		TransformSnapshotComponent* snapshotComponent = dynamic_cast<TransformSnapshotComponent*>(component.get());
		if (snapshotComponent) {
			snapshotComponent->Save();
		}
	}
}

size_t ApplyComponents(Scene& scene, GameObject& gameObject, const json& gameObjectJson) {
	std::vector<Component*> importedComponents;

	TransformComponent* transformComponent = gameObject.GetTransformComponent();
	if (transformComponent) {
		importedComponents.push_back(transformComponent);
	}

	if (!gameObjectJson.contains("components") || !gameObjectJson.at("components").is_array()) {
		RefreshTransformSnapshots(gameObject);
		return importedComponents.size();
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
		if (typeName == "Transform") {
			component = gameObject.GetTransformComponent();
		} else {
			component = FindExistingComponent(gameObject, typeName, importedComponents);
			if (!component) {
				std::unique_ptr<Component> createdComponent = ComponentFactory::GetInstance().Create(typeName);
				component = gameObject.AddComponent(std::move(createdComponent));
				scene.OnEditorComponentAdded(&gameObject, component);
			}
		}

		if (!component) {
			continue;
		}

		if (!ContainsPointer(importedComponents, component)) {
			importedComponents.push_back(component);
		}
		ApplyComponent(*component, componentJson);
	}

	std::vector<Component*> removeTargets;
	for (const std::unique_ptr<Component>& component : gameObject.GetComponents()) {
		if (!component) {
			continue;
		}
		if (!component->CanRemove()) {
			continue;
		}
		if (ContainsPointer(importedComponents, component.get())) {
			continue;
		}
		removeTargets.push_back(component.get());
	}

	for (Component* component : removeTargets) {
		gameObject.RemoveComponent(component);
	}

	RefreshTransformSnapshots(gameObject);
	return importedComponents.size();
}

void ApplyGameObject(Scene& scene, GameObject& gameObject, const json& gameObjectJson, size_t& componentCount) {
	gameObject.SetName(ReadString(gameObjectJson, "name", gameObject.GetName()));
	gameObject.SetActive(ReadBool(gameObjectJson, "active", gameObject.IsActive()));
	componentCount += ApplyComponents(scene, gameObject, gameObjectJson);
}

void RemoveUnimportedGameObjects(Scene& scene, const std::vector<GameObject*>& importedObjects) {
	std::vector<std::unique_ptr<GameObject>>& gameObjects = scene.GetGameObjects();
	auto iterator = gameObjects.begin();
	while (iterator != gameObjects.end()) {
		GameObject* gameObject = iterator->get();
		if (!gameObject || ContainsPointer(importedObjects, gameObject)) {
			++iterator;
			continue;
		}

		gameObject->Finalize();
		iterator = gameObjects.erase(iterator);
	}
}

} // namespace

SceneJsonImporter::ImportResult SceneJsonImporter::ImportScene(Scene& scene, const std::filesystem::path& projectRoot) {
	ImportResult result{};

	std::error_code error;
	std::filesystem::path normalizedProjectRoot = std::filesystem::weakly_canonical(projectRoot, error);
	if (error) {
		normalizedProjectRoot = projectRoot.lexically_normal();
	}

	std::string sceneName = SanitizeFileName(scene.GetSceneName());
	std::filesystem::path sceneDirectory = normalizedProjectRoot / "SceneJson" / sceneName;
	std::filesystem::path sceneJsonPath = sceneDirectory / (sceneName + ".scene.json");
	result.sourceDirectory = sceneDirectory;

	if (!std::filesystem::exists(sceneJsonPath)) {
		result.succeeded = true;
		result.imported = false;
		result.message = "Saved Scene JSON was not found.";
		return result;
	}

	json sceneJson;
	if (!LoadJsonFile(sceneJsonPath, sceneJson, result.message)) {
		result.imported = true;
		return result;
	}

	if (!sceneJson.contains("gameObjects") || !sceneJson.at("gameObjects").is_array()) {
		result.imported = true;
		result.message = "Scene JSON does not contain gameObjects array.";
		return result;
	}

	std::vector<GameObject*> importedObjects;
	const json& gameObjectEntries = sceneJson.at("gameObjects");
	for (size_t objectIndex = 0; objectIndex < gameObjectEntries.size(); ++objectIndex) {
		const json& objectEntry = gameObjectEntries[objectIndex];
		if (!objectEntry.is_object()) {
			continue;
		}

		std::string objectName = ReadString(objectEntry, "name", "GameObject");
		std::string assetPathText = ReadString(objectEntry, "assetPath", "");
		if (assetPathText.empty()) {
			continue;
		}

		std::filesystem::path gameObjectJsonPath = ResolveAssetPath(normalizedProjectRoot, assetPathText);
		json gameObjectJson;
		if (!LoadJsonFile(gameObjectJsonPath, gameObjectJson, result.message)) {
			result.imported = true;
			return result;
		}

		std::string savedObjectName = ReadString(gameObjectJson, "name", objectName);
		GameObject* gameObject = FindExistingGameObject(scene, objectIndex, savedObjectName, importedObjects);
		if (!gameObject) {
			gameObject = scene.CreateGameObject(savedObjectName);
		}
		if (!gameObject) {
			continue;
		}

		ApplyGameObject(scene, *gameObject, gameObjectJson, result.componentCount);
		importedObjects.push_back(gameObject);
		++result.gameObjectCount;
	}

	RemoveUnimportedGameObjects(scene, importedObjects);

	result.succeeded = true;
	result.imported = true;
	result.message = "Imported Scene JSON.";
	return result;
}

} // namespace KujakuEngine
