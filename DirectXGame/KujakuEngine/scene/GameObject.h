#pragma once

#include "Component.h"
#include "../3d/WorldTransform.h"
#include "../components/TransformComponent.h"
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
	explicit GameObject(const std::string& name = "GameObject");
	~GameObject();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// ComponentのUpdateを実行する
	/// </summary>
	void Update();

	/// <summary>
	/// ComponentのDrawを実行する
	/// </summary>
	void Draw();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// EditからPlayへ入る直前の処理
	/// </summary>
	void OnPlayStart();

	/// <summary>
	/// PlayからEditへ戻る時の処理
	/// </summary>
	void OnPlayStop();

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
	WorldTransform& GetTransform();

	/// <summary>
	/// Transformを取得
	/// </summary>
	const WorldTransform& GetTransform() const;

	/// <summary>
	/// 必須Transform Componentを取得
	/// </summary>
	TransformComponent* GetTransformComponent() const { return transformComponent_; }

	/// <summary>
	/// Componentを追加
	/// </summary>
	Component* AddComponent(std::unique_ptr<Component> component);

	/// <summary>
	/// Componentを削除
	/// </summary>
	void RemoveComponent(Component* component);

	/// <summary>
	/// Componentを削除
	/// </summary>
	void RemoveComponentAt(size_t index);

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
	void EnsureTransformComponent();

	std::string name_ = "GameObject";
	bool active_ = true;
	bool initialized_ = false;
	TransformComponent* transformComponent_ = nullptr;
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
