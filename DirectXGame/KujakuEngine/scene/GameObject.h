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
	/// 名前を取得
	/// </summary>
	const std::string& GetName() const { return name_; }

	/// <summary>
	/// 名前を設定
	/// </summary>
	void SetName(const std::string& name) { name_ = name; }

	/// <summary>
	/// 有効状態を取得
	/// </summary>
	bool IsActive() const { return active_; }

	/// <summary>
	/// 有効状態を設定
	/// </summary>
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
	std::string name_ = "GameObject";
	bool active_ = true;
	bool initialized_ = false;
	bool transformInitialized_ = false;
	WorldTransform transform_;
	Component* transformComponent_ = nullptr;
	std::vector<std::unique_ptr<Component>> components_;
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
