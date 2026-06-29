#include "TransformComponent.h"

#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif

namespace KujakuEngine {

namespace {

Vector3 ReadVector3(const nlohmann::json& json, const char* key, const Vector3& defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}

	const nlohmann::json& value = json.at(key);
	if (!value.is_array()) {
		return defaultValue;
	}
	if (value.size() < 3) {
		return defaultValue;
	}
	if (!value[0].is_number() || !value[1].is_number() || !value[2].is_number()) {
		return defaultValue;
	}

	return {value[0].get<float>(), value[1].get<float>(), value[2].get<float>()};
}

} // namespace

void TransformComponent::Initialize() {
	if (initialized_) {
		return;
	}

	transform_.Initialize();
	initialized_ = true;
}

void TransformComponent::DrawInspector() {
#ifdef USE_IMGUI
	ImGui::DragFloat3("Translation", &transform_.translation_.x, 0.01f);
	ImGui::DragFloat3("Rotation", &transform_.rotation_.x, 0.01f);
	ImGui::DragFloat3("Scale", &transform_.scale_.x, 0.01f);

	if (transform_.scale_.x < 0.001f) {
		transform_.scale_.x = 0.001f;
	}
	if (transform_.scale_.y < 0.001f) {
		transform_.scale_.y = 0.001f;
	}
	if (transform_.scale_.z < 0.001f) {
		transform_.scale_.z = 0.001f;
	}
#endif
}

void TransformComponent::WriteJson(nlohmann::json& json) const {
	json["translation"] = {transform_.translation_.x, transform_.translation_.y, transform_.translation_.z};
	json["rotation"] = {transform_.rotation_.x, transform_.rotation_.y, transform_.rotation_.z};
	json["scale"] = {transform_.scale_.x, transform_.scale_.y, transform_.scale_.z};
}

void TransformComponent::ReadJson(const nlohmann::json& json) {
	transform_.translation_ = ReadVector3(json, "translation", transform_.translation_);
	transform_.rotation_ = ReadVector3(json, "rotation", transform_.rotation_);
	transform_.scale_ = ReadVector3(json, "scale", transform_.scale_);

	if (transform_.scale_.x < 0.001f) {
		transform_.scale_.x = 0.001f;
	}
	if (transform_.scale_.y < 0.001f) {
		transform_.scale_.y = 0.001f;
	}
	if (transform_.scale_.z < 0.001f) {
		transform_.scale_.z = 0.001f;
	}
}

} // namespace KujakuEngine
