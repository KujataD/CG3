#include "ComponentFactory.h"
#include <utility>

namespace KujakuEngine {

ComponentFactory& ComponentFactory::GetInstance() {
	static ComponentFactory instance;
	return instance;
}

void ComponentFactory::Register(const std::string& typeName, CreateFunc createFunc) {
	if (typeName.empty() || !createFunc) {
		return;
	}

	if (!creators_.contains(typeName)) {
		typeNames_.push_back(typeName);
	}

	creators_[typeName] = std::move(createFunc);
}

std::unique_ptr<Component> ComponentFactory::Create(const std::string& typeName) const {
	auto iterator = creators_.find(typeName);
	if (iterator == creators_.end()) {
		return nullptr;
	}

	return iterator->second();
}

const std::vector<std::string>& ComponentFactory::GetRegisteredTypeNames() const {
	return typeNames_;
}

} // namespace KujakuEngine
