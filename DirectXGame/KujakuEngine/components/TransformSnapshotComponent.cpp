#include "TransformSnapshotComponent.h"
#include "../scene/GameObject.h"
#include <ostream>
#include <string>

namespace KujakuEngine {

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

void TransformSnapshotComponent::WriteJson(std::ostream& os, int indent) const {
	const std::string padding(static_cast<size_t>(indent), ' ');
	os << padding << "{\n";
	os << padding << "  \"type\": \"" << GetTypeName() << "\",\n";
	os << padding << "  \"enabled\": ";
	if (IsEnabled()) {
		os << "true,\n";
	} else {
		os << "false,\n";
	}
	os << padding << "  \"snapshot\": {\n";
	os << padding << "    \"translation\": ["
	   << editSnapshot_.translation.x << ", "
	   << editSnapshot_.translation.y << ", "
	   << editSnapshot_.translation.z << "],\n";
	os << padding << "    \"rotation\": ["
	   << editSnapshot_.rotation.x << ", "
	   << editSnapshot_.rotation.y << ", "
	   << editSnapshot_.rotation.z << "],\n";
	os << padding << "    \"scale\": ["
	   << editSnapshot_.scale.x << ", "
	   << editSnapshot_.scale.y << ", "
	   << editSnapshot_.scale.z << "]\n";
	os << padding << "  }\n";
	os << padding << "}";
}

} // namespace KujakuEngine
