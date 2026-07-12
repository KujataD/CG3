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
	KUJAKU_API explicit GameObject(const std::string& name = "GameObject");
	KUJAKU_API ~GameObject();


	KUJAKU_API void Initialize();
	KUJAKU_API void Update();
	KUJAKU_API void Draw();

	KUJAKU_API void Finalize();

	/// <summary>
	/// EditからPlayへ入る直前の処理
	/// </summary>
	KUJAKU_API void OnPlayStart();

	/// <summary>
	/// PlayからEditへ戻る時の処理
	/// </summary>
	KUJAKU_API void OnPlayStop();

	KUJAKU_API bool SetParent(GameObject* parent, bool keepWorldPosition = false);

	GameObject* GetParent() const { return parent_; }

	const std::vector<GameObject*>& GetChildren() const { return children_; }

	bool IsRoot() const { return parent_ == nullptr; }

	KUJAKU_API bool IsDescendantOf(const GameObject* ancestor) const;

	KUJAKU_API void UpdateHierarchy();

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

	KUJAKU_API void SetPrefabLink(const std::string& assetPath, const std::string& objectId, const std::string& rootInstanceId, bool isRoot);

	KUJAKU_API void ClearPrefabLink();

	bool IsPrefabInstance() const { return !prefabAssetPath_.empty() && !prefabObjectId_.empty(); }

	bool IsPrefabInstanceRoot() const { return IsPrefabInstance() && isPrefabInstanceRoot_; }

	const std::string& GetPrefabAssetPath() const { return prefabAssetPath_; }

	const std::string& GetPrefabObjectId() const { return prefabObjectId_; }

	const std::string& GetPrefabInstanceRootId() const { return prefabInstanceRootId_; }

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
	/// Tagが一致するかを返します(Unityの CompareTag 相当)。
	/// </summary>
	bool CompareTag(const std::string& tag) const { return tag_ == tag; }

	/// <summary>
	/// 衝突フィルタなどに使うLayerを取得
	/// </summary>
	uint32_t GetLayer() const { return layer_; }

	/// <summary>
	/// 衝突フィルタなどに使うLayerを設定
	/// </summary>
	KUJAKU_API void SetLayer(uint32_t layer);

	bool IsActive() const { return active_; }

	/// <summary>
	/// 親階層を含めた有効状態を取得
	/// </summary>
	KUJAKU_API bool IsActiveInHierarchy() const;

	void SetActive(bool active) { active_ = active; }

	KUJAKU_API WorldTransform& GetTransform();

	KUJAKU_API const WorldTransform& GetTransform() const;

	Component* GetTransformComponent() const { return transformComponent_; }

	KUJAKU_API Component* EnsureTransformComponent();

	KUJAKU_API Component* AddComponent(std::unique_ptr<Component> component);

	KUJAKU_API void RemoveComponent(Component* component);

	KUJAKU_API void RemoveComponentAt(size_t index);

	template <class T, class... Args>
	T* AddComponent(Args&&... args);

	template <class T>
	T* GetComponent();

	std::vector<std::unique_ptr<Component>>& GetComponents() { return components_; }

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
