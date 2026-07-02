#pragma once

#include "../runtime/KujakuApi.h"
#include "Component.h"
#include "../3d/WorldTransform.h"
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace KujakuEngine {

/// <summary>
/// Scene上に配置するObjectの最小単位
/// </summary>
class GameObject {
public:
	/// <summary>
	/// GameObjectを実行します。
	/// </summary>
	KUJAKU_API explicit GameObject(const std::string& name = "GameObject");
	/// <summary>
	/// GameObjectを実行します。
	/// </summary>
	KUJAKU_API ~GameObject();

	/// <summary>
	/// 初期化
	/// </summary>
	KUJAKU_API void Initialize();

	/// <summary>
	/// ComponentのUpdateを実行する
	/// </summary>
	KUJAKU_API void Update();

	/// <summary>
	/// ComponentのDrawを実行する
	/// </summary>
	KUJAKU_API void Draw();

	/// <summary>
	/// 終了処理
	/// </summary>
	KUJAKU_API void Finalize();

	/// <summary>
	/// EditからPlayへ入る直前の処理
	/// </summary>
	KUJAKU_API void OnPlayStart();

	/// <summary>
	/// PlayからEditへ戻る時の処理
	/// </summary>
	KUJAKU_API void OnPlayStop();

	/// <summary>
	/// 親GameObjectを設定
	/// </summary>
	KUJAKU_API bool SetParent(GameObject* parent, bool keepWorldPosition = false);

	/// <summary>
	/// 親GameObjectを取得
	/// </summary>
	GameObject* GetParent() const { return parent_; }

	/// <summary>
	/// 子GameObject一覧を取得
	/// </summary>
	const std::vector<GameObject*>& GetChildren() const { return children_; }

	/// <summary>
	/// ルートGameObjectかどうか
	/// </summary>
	bool IsRoot() const { return parent_ == nullptr; }

	/// <summary>
	/// 指定Objectの子孫かどうか
	/// </summary>
	KUJAKU_API bool IsDescendantOf(const GameObject* ancestor) const;

	/// <summary>
	/// 親子階層をたどってUpdateを実行
	/// </summary>
	KUJAKU_API void UpdateHierarchy();

	/// <summary>
	/// 親子階層をたどってDrawを実行
	/// </summary>
	KUJAKU_API void DrawHierarchy();

	/// <summary>
	/// 親から子へワールド行列を更新
	/// </summary>
	KUJAKU_API void UpdateWorldTransformHierarchy();

	/// <summary>
	/// 親を先に更新して自身のワールド行列を更新
	/// </summary>
	KUJAKU_API void UpdateWorldTransformSelfAndAncestors();

	/// <summary>
	/// Scene保存/復元で使う安定IDを取得
	/// </summary>
	const std::string& GetInstanceId() const { return instanceId_; }

	/// <summary>
	/// Scene保存/復元で使う安定IDを設定
	/// </summary>
	KUJAKU_API void SetInstanceId(const std::string& instanceId);

	/// <summary>
	/// Prefab Instanceとしての参照情報を設定
	/// </summary>
	KUJAKU_API void SetPrefabLink(const std::string& assetPath, const std::string& objectId, const std::string& rootInstanceId, bool isRoot);

	/// <summary>
	/// Prefab Instanceとしての参照情報を解除
	/// </summary>
	KUJAKU_API void ClearPrefabLink();

	/// <summary>
	/// Prefab Instanceかどうか
	/// </summary>
	bool IsPrefabInstance() const { return !prefabAssetPath_.empty() && !prefabObjectId_.empty(); }

	/// <summary>
	/// Prefab InstanceのルートObjectかどうか
	/// </summary>
	bool IsPrefabInstanceRoot() const { return IsPrefabInstance() && isPrefabInstanceRoot_; }

	/// <summary>
	/// 参照元Prefabのパスを取得
	/// </summary>
	const std::string& GetPrefabAssetPath() const { return prefabAssetPath_; }

	/// <summary>
	/// Prefab内Object IDを取得
	/// </summary>
	const std::string& GetPrefabObjectId() const { return prefabObjectId_; }

	/// <summary>
	/// Prefab InstanceルートのinstanceIdを取得
	/// </summary>
	const std::string& GetPrefabInstanceRootId() const { return prefabInstanceRootId_; }

	/// <summary>
	/// 名前を取得
	/// </summary>
	const std::string& GetName() const { return name_; }

	void SetName(const std::string& name) { name_ = name; }

	/// <summary>
	/// ゲームロジック分類用Tagを取得
	/// </summary>
	const std::string& GetTag() const { return tag_; }

	/// <summary>
	/// ゲームロジック分類用Tagを設定
	/// </summary>
	void SetTag(const std::string& tag) { tag_ = tag; }

	/// <summary>
	/// 衝突フィルタなどに使うLayerを取得
	/// </summary>
	uint32_t GetLayer() const { return layer_; }

	/// <summary>
	/// 衝突フィルタなどに使うLayerを設定
	/// </summary>
	KUJAKU_API void SetLayer(uint32_t layer);

	/// <summary>
	/// 有効状態を取得
	/// </summary>
	bool IsActive() const { return active_; }

	/// <summary>
	/// 親階層を含めた有効状態を取得
	/// </summary>
	KUJAKU_API bool IsActiveInHierarchy() const;

	void SetActive(bool active) { active_ = active; }

	/// <summary>
	/// Transformを取得
	/// </summary>
	KUJAKU_API WorldTransform& GetTransform();

	/// <summary>
	/// Transformを取得
	/// </summary>
	KUJAKU_API const WorldTransform& GetTransform() const;

	/// <summary>
	/// 必須Transform Componentを取得
	/// </summary>
	Component* GetTransformComponent() const { return transformComponent_; }

	/// <summary>
	/// 必須Transform Componentを保証
	/// </summary>
	KUJAKU_API Component* EnsureTransformComponent();

	/// <summary>
	/// Componentを追加
	/// </summary>
	KUJAKU_API Component* AddComponent(std::unique_ptr<Component> component);

	/// <summary>
	/// Componentを削除
	/// </summary>
	KUJAKU_API void RemoveComponent(Component* component);

	/// <summary>
	/// Componentを削除
	/// </summary>
	KUJAKU_API void RemoveComponentAt(size_t index);

	/// <summary>
	/// Componentを追加
	/// </summary>
	template <class T, class... Args>
	T* AddComponent(Args&&... args);

	/// <summary>
	/// Componentを取得
	/// </summary>
	template <class T>
	T* GetComponent();

	/// <summary>
	/// Component一覧を取得
	/// </summary>
	std::vector<std::unique_ptr<Component>>& GetComponents() { return components_; }

	/// <summary>
	/// Component一覧を取得
	/// </summary>
	const std::vector<std::unique_ptr<Component>>& GetComponents() const { return components_; }

private:
	std::string instanceId_;
	std::string name_ = "GameObject";
	std::string tag_ = "Untagged";
	uint32_t layer_ = 0;
	bool active_ = true;
	bool initialized_ = false;
	bool transformInitialized_ = false;
	WorldTransform transform_;
	Component* transformComponent_ = nullptr;
	std::vector<std::unique_ptr<Component>> components_;
	GameObject* parent_ = nullptr;
	std::vector<GameObject*> children_;
	std::string prefabAssetPath_;
	std::string prefabObjectId_;
	std::string prefabInstanceRootId_;
	bool isPrefabInstanceRoot_ = false;
};

template <class T, class... Args>
T* GameObject::AddComponent(Args&&... args) {
	static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

	std::unique_ptr<T> component = std::make_unique<T>(std::forward<Args>(args)...);
	Component* raw = AddComponent(std::move(component));
	return static_cast<T*>(raw);
}

template <class T>
T* GameObject::GetComponent() {
	static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

	for (const std::unique_ptr<Component>& component : components_) {
		T* result = dynamic_cast<T*>(component.get());
		if (result) {
			return result;
		}
	}

	return nullptr;
}

} // namespace KujakuEngine
