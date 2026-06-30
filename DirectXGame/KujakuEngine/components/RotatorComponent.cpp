#include "RotatorComponent.h"
#include "../scene/GameObject.h"
#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif // USE_IMGUI

namespace KujakuEngine {

namespace {

float ReadFloat(const nlohmann::json& json, const char* key, float defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	if (!json.at(key).is_number()) {
		return defaultValue;
	}

	return json.at(key).get<float>();
}

} // namespace

void RotatorComponent::Update() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().rotation_.y += speed_;
}

void RotatorComponent::DrawInspector() {
#ifdef USE_IMGUI
	ImGui::DragFloat("Speed", &speed_, 0.001f);
#endif // USE_IMGUI
}

void RotatorComponent::WriteJson(nlohmann::json& json) const {
	json["speed"] = speed_;
}

void RotatorComponent::ReadJson(const nlohmann::json& json) {
	speed_ = ReadFloat(json, "speed", speed_);
}

} // namespace KujakuEngine
