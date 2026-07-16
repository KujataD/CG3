#include "ButtonComponent.h"

#include "../runtime/InspectorUI.h"
#include "../runtime/UIEventBus.h"
#include "../scene/GameObject.h"
#include "ImageComponent.h"
#include <cstring>

namespace KujakuEngine {
namespace {

Vector4 ReadVector4(const nlohmann::json& json, const char* key, const Vector4& defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	const nlohmann::json& value = json.at(key);
	if (!value.is_array() || value.size() < 4) {
		return defaultValue;
	}
	for (int i = 0; i < 4; ++i) {
		if (!value[i].is_number()) {
			return defaultValue;
		}
	}
	return {value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value[3].get<float>()};
}

std::string ReadString(const nlohmann::json& json, const char* key, const std::string& defaultValue) {
	if (!json.contains(key) || !json.at(key).is_string()) {
		return defaultValue;
	}
	return json.at(key).get<std::string>();
}

} // namespace

void ButtonComponent::SyncEventBuffer() {
	std::memset(eventBuffer_.data(), 0, eventBuffer_.size());
	strncpy_s(eventBuffer_.data(), eventBuffer_.size(), onClickEvent_.c_str(), _TRUNCATE);
}

void ButtonComponent::ApplyVisualState(VisualState state) {
	if (!owner_) {
		return;
	}
	ImageComponent* image = owner_->GetComponent<ImageComponent>();
	if (!image) {
		return;
	}
	if (!interactable_) {
		state = VisualState::Disabled;
	}
	switch (state) {
	case VisualState::Highlighted:
		image->SetColor(highlightedColor_);
		break;
	case VisualState::Pressed:
		image->SetColor(pressedColor_);
		break;
	case VisualState::Disabled:
		image->SetColor(disabledColor_);
		break;
	case VisualState::Normal:
	default:
		image->SetColor(normalColor_);
		break;
	}
}

void ButtonComponent::FireOnClick() {
	if (!interactable_) {
		return;
	}
	UIEventBus::Publish(onClickEvent_);
}

void ButtonComponent::DrawInspector() {
#ifdef USE_IMGUI
	if (!eventBufferSynced_) {
		SyncEventBuffer();
		eventBufferSynced_ = true;
	}
	InspectorUI::Checkbox("Interactable", &interactable_);
	InspectorUI::ColorEdit4("Normal", &normalColor_.x);
	InspectorUI::ColorEdit4("Highlighted", &highlightedColor_.x);
	InspectorUI::ColorEdit4("Pressed", &pressedColor_.x);
	InspectorUI::ColorEdit4("Disabled", &disabledColor_.x);
	if (InspectorUI::InputText("On Click Event", eventBuffer_.data(), eventBuffer_.size())) {
		onClickEvent_ = eventBuffer_.data();
	}
#endif // USE_IMGUI
}

void ButtonComponent::WriteJson(nlohmann::json& json) const {
	json["interactable"] = interactable_;
	json["normalColor"] = {normalColor_.x, normalColor_.y, normalColor_.z, normalColor_.w};
	json["highlightedColor"] = {highlightedColor_.x, highlightedColor_.y, highlightedColor_.z, highlightedColor_.w};
	json["pressedColor"] = {pressedColor_.x, pressedColor_.y, pressedColor_.z, pressedColor_.w};
	json["disabledColor"] = {disabledColor_.x, disabledColor_.y, disabledColor_.z, disabledColor_.w};
	json["onClickEvent"] = onClickEvent_;
}

void ButtonComponent::ReadJson(const nlohmann::json& json) {
	if (json.contains("interactable") && json.at("interactable").is_boolean()) {
		interactable_ = json.at("interactable").get<bool>();
	}
	normalColor_ = ReadVector4(json, "normalColor", normalColor_);
	highlightedColor_ = ReadVector4(json, "highlightedColor", highlightedColor_);
	pressedColor_ = ReadVector4(json, "pressedColor", pressedColor_);
	disabledColor_ = ReadVector4(json, "disabledColor", disabledColor_);
	onClickEvent_ = ReadString(json, "onClickEvent", onClickEvent_);
}

void ButtonComponent::OnAfterReadJson() {
	SyncEventBuffer();
	eventBufferSynced_ = true;
}

} // namespace KujakuEngine
