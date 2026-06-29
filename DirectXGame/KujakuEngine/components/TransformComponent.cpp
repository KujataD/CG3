#include "TransformComponent.h"
#include <ostream>
#include <string>

#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif

namespace KujakuEngine {

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

void TransformComponent::WriteJson(std::ostream& os, int indent) const {
	const std::string padding(static_cast<size_t>(indent), ' ');
	os << padding << "{\n";
	os << padding << "  \"type\": \"" << GetTypeName() << "\",\n";
	os << padding << "  \"enabled\": ";
	if (IsEnabled()) {
		os << "true,\n";
	} else {
		os << "false,\n";
	}
	os << padding << "  \"translation\": ["
	   << transform_.translation_.x << ", "
	   << transform_.translation_.y << ", "
	   << transform_.translation_.z << "],\n";
	os << padding << "  \"rotation\": ["
	   << transform_.rotation_.x << ", "
	   << transform_.rotation_.y << ", "
	   << transform_.rotation_.z << "],\n";
	os << padding << "  \"scale\": ["
	   << transform_.scale_.x << ", "
	   << transform_.scale_.y << ", "
	   << transform_.scale_.z << "]\n";
	os << padding << "}";
}

} // namespace KujakuEngine
