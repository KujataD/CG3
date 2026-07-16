#include "CanvasComponent.h"

#include "../runtime/InspectorUI.h"
#include <cmath>

namespace KujakuEngine {
namespace {

float ReadFloat(const nlohmann::json& json, const char* key, float defaultValue) {
	if (!json.contains(key) || !json.at(key).is_number()) {
		return defaultValue;
	}
	return json.at(key).get<float>();
}

Vector2 ReadVector2(const nlohmann::json& json, const char* key, const Vector2& defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	const nlohmann::json& value = json.at(key);
	if (!value.is_array() || value.size() < 2 || !value[0].is_number() || !value[1].is_number()) {
		return defaultValue;
	}
	return {value[0].get<float>(), value[1].get<float>()};
}

} // namespace

CanvasComponent::Layout CanvasComponent::GetLayout(float targetWidth, float targetHeight) const {
	Layout layout;
	float scale = 1.0f;
	if (scaleWithScreenSize_ && referenceResolution_.x > 0.0f && referenceResolution_.y > 0.0f && targetWidth > 0.0f && targetHeight > 0.0f) {
		const float logWidth = std::log2(targetWidth / referenceResolution_.x);
		const float logHeight = std::log2(targetHeight / referenceResolution_.y);
		const float lerped = logWidth + (logHeight - logWidth) * matchWidthHeight_;
		scale = std::pow(2.0f, lerped);
	}
	if (scale < 0.0001f) {
		scale = 0.0001f;
	}
	layout.scaleFactor = scale;
	layout.canvasWidth = targetWidth / scale;
	layout.canvasHeight = targetHeight / scale;
	return layout;
}

void CanvasComponent::DrawInspector() {
#ifdef USE_IMGUI
	InspectorUI::DragFloat("Reference Width", &referenceResolution_.x, 1.0f, 1.0f, 8192.0f);
	InspectorUI::DragFloat("Reference Height", &referenceResolution_.y, 1.0f, 1.0f, 8192.0f);
	InspectorUI::Checkbox("Scale With Screen Size", &scaleWithScreenSize_);
	InspectorUI::DragFloat("Match (W<->H)", &matchWidthHeight_, 0.01f, 0.0f, 1.0f);
	InspectorUI::DragInt("Sort Order", &sortOrder_, 1.0f, -100, 100);
#endif // USE_IMGUI
}

void CanvasComponent::WriteJson(nlohmann::json& json) const {
	json["referenceResolution"] = {referenceResolution_.x, referenceResolution_.y};
	json["matchWidthHeight"] = matchWidthHeight_;
	json["scaleWithScreenSize"] = scaleWithScreenSize_;
	json["sortOrder"] = sortOrder_;
}

void CanvasComponent::ReadJson(const nlohmann::json& json) {
	referenceResolution_ = ReadVector2(json, "referenceResolution", referenceResolution_);
	matchWidthHeight_ = ReadFloat(json, "matchWidthHeight", matchWidthHeight_);
	if (json.contains("scaleWithScreenSize") && json.at("scaleWithScreenSize").is_boolean()) {
		scaleWithScreenSize_ = json.at("scaleWithScreenSize").get<bool>();
	}
	if (json.contains("sortOrder") && json.at("sortOrder").is_number_integer()) {
		sortOrder_ = json.at("sortOrder").get<int>();
	}
}

} // namespace KujakuEngine
