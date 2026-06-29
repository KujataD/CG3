#include "TransformSnapshotComponent.h"
#include "../scene/GameObject.h"

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

void TransformSnapshotComponent::Initialize() {
	Save();
}

void TransformSnapshotComponent::OnPlayStart() {
	Save();
}

void TransformSnapshotComponent::OnPlayStop() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().scale_ = editSnapshot_.scale;
	owner->GetTransform().rotation_ = editSnapshot_.rotation;
	owner->GetTransform().translation_ = editSnapshot_.translation;
}

void TransformSnapshotComponent::Save() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	editSnapshot_.scale = owner->GetTransform().scale_;
	editSnapshot_.rotation = owner->GetTransform().rotation_;
	editSnapshot_.translation = owner->GetTransform().translation_;
}

void TransformSnapshotComponent::WriteJson(nlohmann::json& json) const {
	json["snapshot"]["translation"] = {editSnapshot_.translation.x, editSnapshot_.translation.y, editSnapshot_.translation.z};
	json["snapshot"]["rotation"] = {editSnapshot_.rotation.x, editSnapshot_.rotation.y, editSnapshot_.rotation.z};
	json["snapshot"]["scale"] = {editSnapshot_.scale.x, editSnapshot_.scale.y, editSnapshot_.scale.z};
}

void TransformSnapshotComponent::ReadJson(const nlohmann::json& json) {
	const nlohmann::json* snapshotJson = &json;
	if (json.contains("snapshot") && json.at("snapshot").is_object()) {
		snapshotJson = &json.at("snapshot");
	}

	editSnapshot_.translation = ReadVector3(*snapshotJson, "translation", editSnapshot_.translation);
	editSnapshot_.rotation = ReadVector3(*snapshotJson, "rotation", editSnapshot_.rotation);
	editSnapshot_.scale = ReadVector3(*snapshotJson, "scale", editSnapshot_.scale);
}

void TransformSnapshotComponent::OnAfterReadJson() {
	Save();
}

} // namespace KujakuEngine
