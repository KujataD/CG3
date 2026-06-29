#include "RotatorComponent.h"
#include "../scene/GameObject.h"
#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#endif // USE_IMGUI
#include <ostream>
#include <string>

namespace KujakuEngine {

void RotatorComponent::Update() {
	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	owner->GetTransform().rotation_.x += speed_;
}

void RotatorComponent::DrawInspector() {
#ifdef USE_IMGUI
	ImGui::DragFloat("Speed", &speed_, 0.001f);
#endif // USE_IMGUI
}

void RotatorComponent::WriteJson(std::ostream& os, int indent) const {
	const std::string padding(static_cast<size_t>(indent), ' ');
	os << padding << "{\n";
	os << padding << "  \"type\": \"" << GetTypeName() << "\",\n";
	os << padding << "  \"enabled\": ";
	if (IsEnabled()) {
		os << "true,\n";
	} else {
		os << "false,\n";
	}
	os << padding << "  \"speed\": " << speed_ << "\n";
	os << padding << "}";
}

} // namespace KujakuEngine
