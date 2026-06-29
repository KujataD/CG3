#include "GameObject.h"
#include <algorithm>

namespace KujakuEngine {

GameObject::GameObject(const std::string& name) : name_(name) {
	EnsureTransformComponent();
}

GameObject::~GameObject() = default;

void GameObject::Initialize() {
	if (initialized_) {
		return;
	}

	EnsureTransformComponent();

	for (const std::unique_ptr<Component>& component : components_) {
		if (component) {
			component->Initialize();
		}
	}

	initialized_ = true;
}

void GameObject::Update() {
	if (!active_) {
		return;
	}

	for (const std::unique_ptr<Component>& component : components_) {
		if (component && component->IsEnabled()) {
			component->Update();
		}
	}
}

void GameObject::Draw() {
	if (!active_) {
		return;
	}

	for (const std::unique_ptr<Component>& component : components_) {
		if (component && component->IsEnabled()) {
			component->Draw();
		}
	}
}

void GameObject::Finalize() {
	for (const std::unique_ptr<Component>& component : components_) {
		if (component) {
			component->SetOwner(nullptr);
		}
	}
	components_.clear();
	transformComponent_ = nullptr;
	initialized_ = false;
}

void GameObject::OnPlayStart() {
	for (const std::unique_ptr<Component>& component : components_) {
		if (component && component->IsEnabled()) {
			component->OnPlayStart();
		}
	}
}

void GameObject::OnPlayStop() {
	for (const std::unique_ptr<Component>& component : components_) {
		if (component && component->IsEnabled()) {
			component->OnPlayStop();
		}
	}
}

WorldTransform& GameObject::GetTransform() {
	EnsureTransformComponent();
	return transformComponent_->GetTransform();
}

const WorldTransform& GameObject::GetTransform() const {
	const_cast<GameObject*>(this)->EnsureTransformComponent();
	return transformComponent_->GetTransform();
}

Component* GameObject::AddComponent(std::unique_ptr<Component> component) {
	if (!component) {
		return nullptr;
	}

	if (!component->AllowMultiple()) {
		const char* typeName = component->GetTypeName();
		for (const std::unique_ptr<Component>& current : components_) {
			if (current && std::string(current->GetTypeName()) == typeName) {
				return current.get();
			}
		}
	}

	Component* raw = component.get();
	raw->SetOwner(this);
	components_.push_back(std::move(component));

	TransformComponent* transformComponent = dynamic_cast<TransformComponent*>(raw);
	if (transformComponent) {
		transformComponent_ = transformComponent;
	}

	if (initialized_) {
		raw->Initialize();
	}

	return raw;
}

void GameObject::RemoveComponent(Component* component) {
	if (!component) {
		return;
	}

	auto iterator = std::find_if(components_.begin(), components_.end(), [component](const std::unique_ptr<Component>& current) {
		return current.get() == component;
	});

	if (iterator == components_.end()) {
		return;
	}

	if (!(*iterator)->CanRemove()) {
		return;
	}

	(*iterator)->SetOwner(nullptr);
	components_.erase(iterator);
}

void GameObject::RemoveComponentAt(size_t index) {
	if (index >= components_.size()) {
		return;
	}

	if (!components_[index]->CanRemove()) {
		return;
	}

	components_[index]->SetOwner(nullptr);
	components_.erase(components_.begin() + index);
}

void GameObject::EnsureTransformComponent() {
	if (transformComponent_) {
		return;
	}

	transformComponent_ = AddComponent<TransformComponent>();
}

} // namespace KujakuEngine
