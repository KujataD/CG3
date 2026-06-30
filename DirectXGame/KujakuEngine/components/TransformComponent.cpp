#include "TransformComponent.h"
#include "../scene/GameObject.h"

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

WorldTransform& TransformComponent::GetTransform() {
	GameObject* owner = GetOwner();
	if (owner) {
		return owner->GetTransform();
	}
	return fallbackTransform_;
}

const WorldTransform& TransformComponent::GetTransform() const {
	const GameObject* owner = GetOwner();
	if (owner) {
		return owner->GetTransform();
	}
	return fallbackTransform_;
}

void TransformComponent::Initialize() {
	if (initialized_) {
		return;
	}

	initialized_ = true;
}

void TransformComponent::DrawInspector() {
#ifdef USE_IMGUI
	WorldTransform& transform = GetTransform();
	ImGui::DragFloat3("Translation", &transform.translation_.x, 0.01f);
	ImGui::DragFloat3("Rotation", &transform.rotation_.x, 0.01f);
	ImGui::DragFloat3("Scale", &transform.scale_.x, 0.01f);

	if (transform.scale_.x < 0.001f) {
		transform.scale_.x = 0.001f;
	}
	if (transform.scale_.y < 0.001f) {
		transform.scale_.y = 0.001f;
	}
	if (transform.scale_.z < 0.001f) {
		transform.scale_.z = 0.001f;
	}
#endif
}

void TransformComponent::WriteJson(nlohmann::json& json) const {
	const WorldTransform& transform = GetTransform();
	json["translation"] = {transform.translation_.x, transform.translation_.y, transform.translation_.z};
	json["rotation"] = {transform.rotation_.x, transform.rotation_.y, transform.rotation_.z};
	json["scale"] = {transform.scale_.x, transform.scale_.y, transform.scale_.z};
}

void TransformComponent::ReadJson(const nlohmann::json& json) {
	WorldTransform& transform = GetTransform();
	transform.translation_ = ReadVector3(json, "translation", transform.translation_);
	transform.rotation_ = ReadVector3(json, "rotation", transform.rotation_);
	transform.scale_ = ReadVector3(json, "scale", transform.scale_);

	if (transform.scale_.x < 0.001f) {
		transform.scale_.x = 0.001f;
	}
	if (transform.scale_.y < 0.001f) {
		transform.scale_.y = 0.001f;
	}
	if (transform.scale_.z < 0.001f) {
		transform.scale_.z = 0.001f;
	}
}

} // namespace KujakuEngine
