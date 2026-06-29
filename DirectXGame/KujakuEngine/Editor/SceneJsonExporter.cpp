#include "SceneJsonExporter.h"
#include "../scene/Component.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <system_error>
#include <vector>

namespace KujakuEngine {

namespace {

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
		return "GameObject";
	}

	return sanitized;
}

uint64_t HashText(const std::string& text, uint64_t seed) {
	uint64_t hash = 1469598103934665603ull ^ seed;
	for (unsigned char character : text) {
		hash ^= static_cast<uint64_t>(character);
		hash *= 1099511628211ull;
	}
	return hash;
}

std::string MakePseudoGuid(const std::string& key) {
	uint64_t first = HashText(key, 0xcbf29ce484222325ull);
	uint64_t second = HashText(key, 0x9e3779b97f4a7c15ull);

	std::ostringstream os;
	os << std::hex << std::setfill('0') << std::setw(16) << first << std::setw(16) << second;
	return os.str();
}

std::string MakeRelativeAssetPath(const std::filesystem::path& projectRoot, const std::filesystem::path& assetPath) {
	std::error_code error;
	std::filesystem::path relative = std::filesystem::relative(assetPath, projectRoot, error);
	if (error) {
		return assetPath.generic_string();
	}
	return relative.generic_string();
}

bool WriteTextFile(const std::filesystem::path& path, const std::string& text, std::string& message) {
	std::ofstream file(path, std::ios::binary);
	if (!file) {
		message = "Failed to open file: " + path.string();
		return false;
	}

	file << text;
	if (!file) {
		message = "Failed to write file: " + path.string();
		return false;
	}

	return true;
}

std::string BuildFolderMeta(const std::filesystem::path& projectRoot, const std::filesystem::path& folderPath) {
	std::string assetPath = MakeRelativeAssetPath(projectRoot, folderPath);
	std::ostringstream os;
	os << "fileFormatVersion: 2\n";
	os << "guid: " << MakePseudoGuid(assetPath) << "\n";
	os << "folderAsset: yes\n";
	os << "DefaultImporter:\n";
	os << "  externalObjects: {}\n";
	os << "  userData: KujakuEngine generated folder meta\n";
	os << "  assetBundleName:\n";
	os << "  assetBundleVariant:\n";
	return os.str();
}

std::string BuildAssetMeta(const std::filesystem::path& projectRoot, const std::filesystem::path& assetPath, const std::string& assetType) {
	std::string relativePath = MakeRelativeAssetPath(projectRoot, assetPath);
	std::ostringstream os;
	os << "fileFormatVersion: 2\n";
	os << "guid: " << MakePseudoGuid(relativePath) << "\n";
	os << "KujakuJsonImporter:\n";
	os << "  externalObjects: {}\n";
	os << "  assetType: " << assetType << "\n";
	os << "  assetPath: " << relativePath << "\n";
	os << "  userData: KujakuEngine generated meta\n";
	os << "  assetBundleName:\n";
	os << "  assetBundleVariant:\n";
	return os.str();
}

bool WriteFolderMeta(const std::filesystem::path& projectRoot, const std::filesystem::path& folderPath, std::string& message) {
	std::filesystem::path metaPath = folderPath;
	metaPath += ".meta";
	return WriteTextFile(metaPath, BuildFolderMeta(projectRoot, folderPath), message);
}

bool WriteAssetMeta(const std::filesystem::path& projectRoot, const std::filesystem::path& assetPath, const std::string& assetType, std::string& message) {
	std::filesystem::path metaPath = assetPath;
	metaPath += ".meta";
	return WriteTextFile(metaPath, BuildAssetMeta(projectRoot, assetPath, assetType), message);
}

std::string BuildGameObjectJson(const GameObject& gameObject) {
	std::ostringstream os;
	os << "{\n";
	os << "  \"assetType\": \"GameObject\",\n";
	os << "  \"name\": \"" << EscapeJsonString(gameObject.GetName()) << "\",\n";
	os << "  \"active\": ";
	if (gameObject.IsActive()) {
		os << "true,\n";
	} else {
		os << "false,\n";
	}
	os << "  \"components\": [\n";

	const std::vector<std::unique_ptr<Component>>& components = gameObject.GetComponents();
	for (size_t componentIndex = 0; componentIndex < components.size(); ++componentIndex) {
		const std::unique_ptr<Component>& component = components[componentIndex];
		if (!component) {
			continue;
		}

		component->WriteJson(os, 4);
		if (componentIndex + 1 < components.size()) {
			os << ",";
		}
		os << "\n";
	}

	os << "  ]\n";
	os << "}\n";
	return os.str();
}

std::string BuildSceneJson(
    const Scene& scene,
    const std::filesystem::path& projectRoot,
    const std::vector<std::filesystem::path>& gameObjectPaths) {
	std::ostringstream os;
	os << "{\n";
	os << "  \"assetType\": \"Scene\",\n";
	os << "  \"name\": \"" << EscapeJsonString(scene.GetSceneName()) << "\",\n";
	os << "  \"gameObjects\": [\n";

	const std::vector<std::unique_ptr<GameObject>>& gameObjects = scene.GetGameObjects();
	for (size_t objectIndex = 0; objectIndex < gameObjects.size(); ++objectIndex) {
		const std::unique_ptr<GameObject>& gameObject = gameObjects[objectIndex];
		if (!gameObject) {
			continue;
		}

		os << "    {\n";
		os << "      \"name\": \"" << EscapeJsonString(gameObject->GetName()) << "\",\n";
		os << "      \"assetPath\": \"" << EscapeJsonString(MakeRelativeAssetPath(projectRoot, gameObjectPaths[objectIndex])) << "\"\n";
		os << "    }";
		if (objectIndex + 1 < gameObjects.size()) {
			os << ",";
		}
		os << "\n";
	}

	os << "  ]\n";
	os << "}\n";
	return os.str();
}

} // namespace

SceneJsonExporter::ExportResult SceneJsonExporter::ExportScene(const Scene& scene, const std::filesystem::path& projectRoot) {
	ExportResult result{};

	std::error_code error;
	if (projectRoot.empty()) {
		result.message = "ProjectDir is empty.";
		return result;
	}

	std::filesystem::path normalizedProjectRoot = std::filesystem::weakly_canonical(projectRoot, error);
	if (error) {
		normalizedProjectRoot = projectRoot.lexically_normal();
	}

	std::string sceneName = SanitizeFileName(scene.GetSceneName());
	std::filesystem::path sceneJsonRoot = normalizedProjectRoot / "SceneJson";
	std::filesystem::path sceneDirectory = sceneJsonRoot / sceneName;
	std::filesystem::path gameObjectDirectory = sceneDirectory / "GameObjects";

	std::filesystem::create_directories(gameObjectDirectory, error);
	if (error) {
		result.message = "Failed to create SceneJson directory: " + error.message();
		return result;
	}

	if (!WriteFolderMeta(normalizedProjectRoot, sceneJsonRoot, result.message)) {
		return result;
	}
	if (!WriteFolderMeta(normalizedProjectRoot, sceneDirectory, result.message)) {
		return result;
	}
	if (!WriteFolderMeta(normalizedProjectRoot, gameObjectDirectory, result.message)) {
		return result;
	}

	const std::vector<std::unique_ptr<GameObject>>& gameObjects = scene.GetGameObjects();
	std::vector<std::filesystem::path> gameObjectPaths;
	gameObjectPaths.resize(gameObjects.size());

	for (size_t objectIndex = 0; objectIndex < gameObjects.size(); ++objectIndex) {
		const std::unique_ptr<GameObject>& gameObject = gameObjects[objectIndex];
		if (!gameObject) {
			continue;
		}

		std::ostringstream fileName;
		fileName << std::setfill('0') << std::setw(4) << objectIndex << "_" << SanitizeFileName(gameObject->GetName()) << ".gameobject.json";

		std::filesystem::path gameObjectPath = gameObjectDirectory / fileName.str();
		gameObjectPaths[objectIndex] = gameObjectPath;

		if (!WriteTextFile(gameObjectPath, BuildGameObjectJson(*gameObject), result.message)) {
			return result;
		}
		if (!WriteAssetMeta(normalizedProjectRoot, gameObjectPath, "GameObject", result.message)) {
			return result;
		}

		++result.gameObjectFileCount;
	}

	std::filesystem::path sceneJsonPath = sceneDirectory / (sceneName + ".scene.json");
	if (!WriteTextFile(sceneJsonPath, BuildSceneJson(scene, normalizedProjectRoot, gameObjectPaths), result.message)) {
		return result;
	}
	if (!WriteAssetMeta(normalizedProjectRoot, sceneJsonPath, "Scene", result.message)) {
		return result;
	}

	result.succeeded = true;
	result.outputDirectory = sceneDirectory;
	result.sceneFileCount = 1;
	result.message = "Exported Scene JSON.";
	return result;
}

} // namespace KujakuEngine
