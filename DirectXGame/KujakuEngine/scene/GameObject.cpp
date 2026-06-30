#include "GameObject.h"
#include "ComponentFactory.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace KujakuEngine {

namespace {

std::string GenerateInstanceId() {
	static std::atomic<uint64_t> counter = 0;
	static std::random_device randomDevice;
	static std::mt19937_64 randomEngine(randomDevice());

	const uint64_t now = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
	const uint64_t sequence = ++counter;
	const uint64_t randomValue = randomEngine();

	std::ostringstream os;
	os << "go_";
	os << std::hex << std::setfill('0');
	os << std::setw(16) << (now ^ randomValue);
	os << std::setw(16) << (sequence ^ (randomValue >> 1));
	return os.str();
}

} // namespace

GameObject::GameObject(const std::string& name) : instanceId_(GenerateInstanceId()), name_(name) {
	EnsureTransformComponent();
}

GameObject::~GameObject() = default;

void GameObject::SetInstanceId(const std::string& instanceId) {
	// 既存JSONにIDがない場合や壊れている場合は、生成済みIDを維持する。
	if (instanceId.empty()) {
		return;
	}

	instanceId_ = instanceId;
}

void GameObject::Initialize() {
	if (initialized_) {
		return;
	}

	EnsureTransformComponent();
	if (!transformInitialized_) {
		transform_.Initialize();
		transformInitialized_ = true;
	}

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
	return transform_;
}

const WorldTransform& GameObject::GetTransform() const {
	const_cast<GameObject*>(this)->EnsureTransformComponent();
	return transform_;
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

	if (raw->IsTransformComponent()) {
		transformComponent_ = raw;
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

Component* GameObject::EnsureTransformComponent() {
	if (transformComponent_) {
		return transformComponent_;
	}

	for (const std::unique_ptr<Component>& component : components_) {
		if (!component) {
			continue;
		}
		if (component->IsTransformComponent()) {
			transformComponent_ = component.get();
			return transformComponent_;
		}
	}

	std::unique_ptr<Component> transformComponent = ComponentFactory::GetInstance().Create("Transform");
	if (!transformComponent) {
		transformComponent = ComponentFactory::GetInstance().Create("TransformComponent");
	}

	Component* added = AddComponent(std::move(transformComponent));
	if (added && added->IsTransformComponent()) {
		transformComponent_ = added;
	}

	return transformComponent_;
}

} // namespace KujakuEngine
