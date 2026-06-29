#include "Blackboard.h"

namespace KujakuEngine {

void Blackboard::SetValue(const std::string& key, const BlackboardValue& value) {
	data_[key] = value;
}

bool Blackboard::HasKey(const std::string& key) const {
	return data_.find(key) != data_.end();
}

void Blackboard::RemoveValue(const std::string& key) {
	data_.erase(key);
}

void Blackboard::Clear() {
	data_.clear();
}

const BlackboardValue* Blackboard::TryGetRawValue(const std::string& key) const {
	auto it = data_.find(key);
	if (it == data_.end()) {
		return nullptr;
	}

	return &it->second;
}

} // namespace KujakuEngine
