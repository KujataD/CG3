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
	/// GameObjectと子階層を指定Prefabファイルへ保存する
	/// </summary>
	KUJAKU_API static SaveResult SaveToPath(const GameObject& rootObject, const std::filesystem::path& prefabPath);

	/// <summary>
	/// Prefabファイルから新しいGameObject階層をSceneへ生成する
	/// </summary>
	KUJAKU_API static InstantiateResult Instantiate(Scene& scene, const std::filesystem::path& prefabPath, bool linkInstance = true);

	/// <summary>
	/// 既存GameObject階層をPrefab Instanceとして関連付ける
	/// </summary>
	KUJAKU_API static void BindHierarchyToPrefab(GameObject& rootObject, const std::filesystem::path& prefabPath);

	/// <summary>
	/// Prefab Instanceを参照元Prefabの内容へ戻す
	/// </summary>
	KUJAKU_API static InstantiateResult RevertPrefabInstance(Scene& scene, GameObject& instanceObject);

	/// <summary>
	/// Prefab Instanceの変更をPrefabファイルへ適用する
	/// </summary>
	KUJAKU_API static SaveResult ApplyPrefabInstance(Scene& scene, GameObject& instanceObject);

	/// <summary>
	/// Prefab Instanceの関連付けを解除する
	/// </summary>
	KUJAKU_API static bool UnpackPrefabInstance(Scene& scene, GameObject& instanceObject);

	/// <summary>
	/// 選択ObjectからPrefab Instanceルートを取得する
	/// </summary>
	KUJAKU_API static GameObject* FindPrefabInstanceRoot(Scene& scene, GameObject& object);

	/// <summary>
	/// Scene内の同じPrefab InstanceをPrefabファイルから更新する
	/// </summary>
	KUJAKU_API static size_t RefreshInstancesFromPrefab(Scene& scene, const std::filesystem::path& prefabPath, GameObject* ignoreRoot = nullptr);
};

} // namespace KujakuEngine
