#pragma once

#include "../runtime/KujakuApi.h"
#include <cstddef>
#include <filesystem>
#include <string>

namespace KujakuEngine {

class GameObject;
class Scene;

/// <summary>
/// GameObject構成をPrefabファイルとして保存・復元する
/// </summary>
class PrefabAsset {
public:
	/// <summary>
	/// Prefab保存結果
	/// </summary>
	struct SaveResult {
		bool succeeded = false;
		std::filesystem::path outputPath;
		size_t gameObjectCount = 0;
		size_t componentCount = 0;
		std::string message;
	};

	/// <summary>
	/// Prefab生成結果
	/// </summary>
	struct InstantiateResult {
		bool succeeded = false;
		GameObject* rootObject = nullptr;
		size_t gameObjectCount = 0;
		size_t componentCount = 0;
		std::string message;
	};

	/// <summary>
	/// GameObjectと子階層をProjectDir配下のPrefabsへ保存する
	/// </summary>
	KUJAKU_API static SaveResult SaveAsPrefab(const GameObject& rootObject, const std::filesystem::path& projectRoot);

	/// <summary>
	/// Prefabファイルから新しいGameObject階層をSceneへ生成する
	/// </summary>
	KUJAKU_API static InstantiateResult Instantiate(Scene& scene, const std::filesystem::path& prefabPath);
};

} // namespace KujakuEngine
